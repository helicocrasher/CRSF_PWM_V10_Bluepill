# mySerial Class for STM32 HAL

## Overview
`mySerial` is a flexible, non-blocking UART communication class for STM32 microcontrollers using the HAL library. It supports both transmission (TX) and reception (RX) simultaneously, managed via interrupt-driven methods. The class encapsulates internal FIFO buffers with state management and provides a robust API for reading, writing, and monitoring serial communication.

## Features
- Supports simultaneous TX and RX operation
- Non-blocking interrupt-driven UART communication
- Internal FIFO buffers for data management
- Configurable FIFO and UART buffer sizes
- **Encapsulated state management** - ready flags are internal to the class
- **Initialization guard** - prevents uninitialized usage errors
- TX mode: FIFO will not overrun (prevents data loss)
- RX mode: FIFO may overrun if data is not read fast enough (oldest data is dropped)
- Safe method guards to prevent crashes from uninitialized objects

## API

### Constructor / Destructor
```cpp
mySerial();           // Initialize with null pointers and zero sizes
~mySerial();          // Clean up dynamically allocated FIFO buffers
```

### Initialization
```cpp
void init(UART_HandleTypeDef *huart, size_t fifo_buffer_size = 256, size_t tx_UART_buffer_size = 4);
```
- **Parameters:**
  - `huart`: Pointer to the UART handle
  - `fifo_buffer_size`: Size of the internal FIFO buffer for RX/TX (default 256 bytes)
  - `tx_UART_buffer_size`: Size of the temporary TX buffer for HAL transmission (default 4 bytes)
- **Behavior:**
  - Allocates FIFO buffers on heap
  - Sets internal ready flags to initial states
  - Aborts any existing UART operations
  - Starts UART reception in interrupt mode (single byte RX-IT)
  - Must be called **exactly once** before using any other methods
  - Sets `m_initialized = true` when complete

### Safety Check
```cpp
bool isInitialized() const;
```
- Returns `true` if `init()` has been successfully called
- Returns `false` if object is uninitialized
- **Recommended:** Check before critical operations in application code

### Write Data (TX)
```cpp
size_t write(const uint8_t *input_array, size_t len);
```
- Pushes up to `len` bytes from `input_array` into the TX FIFO buffer
- Returns the number of bytes actually written (may be less if FIFO is full)
- **Behavior:** Automatically calls `send()` to initiate transmission if UART is ready
- **Guard:** Returns 0 if not initialized

### TX Callback Handler
```cpp
size_t TX_callBackPull();
```
- **Call location:** Inside `HAL_UART_TxCpltCallback()` after transmission completes
- **Behavior:** 
  - Pulls next chunk of data from TX FIFO and starts another transmission
  - Returns number of bytes sent to UART (0 if FIFO is empty)
- **Usage:** If return value is 0, call `set_ready_TX()` to mark UART as idle
- **Guard:** Returns 0 if not initialized

### RX Callback Handler
```cpp
int8_t receive();
```
- **Call location:** Inside `HAL_UART_RxCpltCallback()` after data received
- **Behavior:**
  - Pushes received byte(s) from UART buffer into RX FIFO
  - Re-arms UART RX interrupt for next byte
- **Return:** Number of bytes in RX FIFO, or -1 if not initialized
- **Guard:** Returns -1 if not initialized

### Read Data (RX)
```cpp
size_t read(uint8_t *output_array, size_t len);
```
- Pops up to `len` bytes from RX FIFO into `output_array`
- Returns the number of bytes actually read (may be less if FIFO is empty)
- **Guard:** Returns 0 if not initialized

### Check Available Data
```cpp
size_t available();
```
- Returns the number of bytes currently in RX FIFO
- **Warning:** If `available() == fifo_buffer_size`, RX FIFO may have overflowed (oldest data lost)

### Flush TX Buffer
```cpp
int8_t flush(uint32_t flush_timeout);
```
- Waits until TX FIFO is empty and UART transmission is complete
- Blocks up to `flush_timeout` milliseconds
- **Returns:** 0 on success, -1 on timeout
- **Behavior on timeout:** Aborts UART transmission and clears TX FIFO
- **Guard:** Returns -1 if not initialized

### Internal State Setters (for UART callbacks)
```cpp
void set_ready_TX();  // Called by HAL_UART_TxCpltCallback when TX completes
void set_ready_RX();  // Called by HAL_UART_RxCpltCallback when RX completes
```
- **Internal use only** - encapsulates ready flag management
- No need for external global variables

### Internal State Checkers
```cpp
bool is_idle_TX();    // Returns true if TX FIFO empty AND TX ready
bool is_idle_RX();    // Returns true if RX FIFO empty AND RX ready
```
- Used by `flush()` and application logic to check transmission status

### Restart RX
```cpp
int8_t restart_RX();
```
- Aborts current RX and resets both FIFO buffers
- Restarts UART reception interrupt
- **Returns:** 0 on success, -1 if not initialized
- **Use case:** Recovery from communication errors or link restart

### Access UART RX Buffer
```cpp
uint8_t* get_uart_rx_buffer();
```
- Returns pointer to the UART RX interrupt buffer
- Used by callbacks to identify which buffer triggered the interrupt

## Internal FIFO Management (Private)
- `fifo_free_space(bool isTX)`: Returns available space in TX or RX FIFO
- `fifo_data_length(bool isTX)`: Returns number of bytes in TX or RX FIFO
- `fifo_push(bool isTX, uint8_t c)`: Pushes a byte into FIFO (overwrites oldest on RX overflow)
- `fifo_pop(bool isTX)`: Pops a byte from FIFO

## UART Callbacks Integration

### In HAL_UART_TxCpltCallback
```cpp
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // UART1 handling
        crsfSerialWrapper.set_ready_TX();  // Mark ready if nothing to send
        // or pull next chunk:
        if (crsfSerialWrapper.TX_callBackPull() == 0) {
            crsfSerialWrapper.set_ready_TX();
        }
    }
}
```

### In HAL_UART_RxCpltCallback
```cpp
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        crsfSerialWrapper.set_ready_RX();
        crsfSerialWrapper.receive();  // Push data to FIFO and re-arm RX
    }
}
```

## Usage Example

### Basic Initialization
```cpp
#include "mySerial.h"

// Create serial instance
mySerial debug_serial;

void setup() {
    // Initialize with UART2, 256-byte FIFO, 4-byte TX buffer
    debug_serial.init(&huart2, 256, 4);
    
    // Check if initialized successfully
    if (!debug_serial.isInitialized()) {
        // Handle initialization error
        return;
    }
}
```

### Writing Data (TX)
```cpp
void send_message() {
    uint8_t tx_data[] = "Hello, UART!";
    size_t written = debug_serial.write(tx_data, sizeof(tx_data) - 1);
    
    if (written > 0) {
        printf("Sent %d bytes\n", written);
    }
}
```

### Reading Data (RX)
```cpp
void read_message() {
    uint8_t rx_data[16];
    size_t available = debug_serial.available();
    
    if (available > 0) {
        size_t bytes_read = debug_serial.read(rx_data, sizeof(rx_data));
        // Process received data
    }
}
```

### Flush and Wait for TX Complete
```cpp
void ensure_sent() {
    // Wait up to 1000ms for all data to be sent
    int8_t result = debug_serial.flush(1000);
    
    if (result == 0) {
        printf("All data sent successfully\n");
    } else {
        printf("TX timeout - data may not have been fully sent\n");
    }
}
```

### Recovery / Restart RX
```cpp
void recover_communication() {
    debug_serial.restart_RX();
    printf("RX restarted, ready for new data\n");
}
```

## Safety Features

### Initialization Guard
- All methods check `m_initialized` flag before accessing buffers or UART
- Methods return safely (0 or -1) if called before `init()`
- Prevents null pointer dereferences and access violations

### Encapsulated State Management
- Ready flags (`m_huart_tx_ready`, `m_huart_rx_ready`) are private members
- No external global flag variables needed
- State is set only by `init()` and the setter methods
- Cleaner architecture and reduced coupling

### Buffer Protection
- TX FIFO: Will not overrun (drops new data if full, preserves existing)
- RX FIFO: May overrun if not read fast enough (oldest data dropped)
- Both use circular buffer logic to prevent memory corruption

## Best Practices

1. **Always check initialization:**
   ```cpp
   if (serial.isInitialized()) {
       serial.write(data, len);
   }
   ```

2. **Call callbacks in HAL interrupt handlers:**
   - `set_ready_TX()` / `set_ready_RX()` in respective callbacks
   - `TX_callBackPull()` and `receive()` at appropriate times

3. **Monitor RX FIFO overflow:**
   ```cpp
   if (serial.available() == FIFO_SIZE) {
       // Potential overflow, data may have been lost
       // Consider calling restart_RX() for recovery
   }
   ```

4. **Flush before critical operations:**
   ```cpp
   serial.flush(1000);  // Ensure all TX data is sent
   // Now safe to power down, switch modes, etc.
   ```

5. **Handle uninitialized returns gracefully:**
   ```cpp
   int8_t ret = serial.restart_RX();
   if (ret != 0) {
       // Not initialized - handle error
   }
   ```
