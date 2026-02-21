# mySerial Class for STM32 HAL

## Overview
`mySerial` is a flexible, non-blocking UART communication class for STM32 microcontrollers using the HAL library. It supports both transmission (TX) and reception (RX) simultaneously, managed via interrupt-driven methods. The class uses an internal FIFO buffers for data queuing and provides simple APIs for reading, writing, and updating serial communication.

## Features
- Supports TX and RX operation
- Non-blocking UART communication using HAL interrupts
- Internal FIFO buffers for data management
- Configurable FIFO and UART buffer sizes
- Simple API for writing, reading, and updating the serial state
- Prevents UART buffer overrun
- Manages the UART ready state
- In TX mode the FIFO will not overrun 
- In RX mode the FIFO might overrun if data is not read fast enough in that case the oldest data will be over written

## API

### Constructor / Destructor
```cpp
mySerial();
~mySerial();
```

### Initialization
```cpp
void init(UART_HandleTypeDef *huart, bool *huart_TX_ready, bool *huart_RX_raedy*, size_t fifo_buffer_size = 256, size_t UART_TX_buffer_size = 4);
```
- `huart`: Pointer to the UART handle
- `huart_TX_ready`: Pointer to a flag indicating UART ready state (should be set in UART callbacks)
- `huart_RX_ready`: Pointer to a flag indicating UART ready state (should be set in UART callbacks)
- `fifo_buffer_size`: Size of the internal FIFO buffer (default 256)
- `UART_TX_buffer_size`: Size of the temporary UART buffer (default 4)
- init() also starts reception of data. Single bytes are are read interrupt driven and then copied to RX FIFO

### Write Data (TX mode)
```cpp
size_t write(const uint8_t *input_array, size_t len);
```
- Copies up to `len` bytes from `input_array` into the FIFO buffer
- Returns the number of bytes actually written (may be less if FIFO is full)
- NEW: if no transmission is ongoing (huart_IT_ready) the send() method is called 
-->self start.

### TX_callBackPull NEW!
```cpp
size_t TX_callBackPull(void)
```
- method to call from the HAL_UART_xCpltCallback interrupt callback 
- if data is in the FIFO a next transmission is started
- return value the number of bytes put to transmission max UART_buffersize, min 0
- (the HAL_UART_TxCpltCallback must set the huart_IT_ready after TX_callBackPull()==0)

### Read Data 
```cpp
size_t read(uint8_t *output_array, size_t len);
```
- Reads up to `len` bytes from the FIFO buffer into `output_array`
- Returns the number of bytes actually read (may be less if FIFO is empty)

### Available Data (makes sense for RX mode)
```cpp
size_t available();
- Returns the number of bytes in the FIFO. in RX mode if available() = fifo_buffer_size data might have been lost 

### Update Serial State
```cpp
int8_t updateSerial();
```
- For TX: sends data if UART is ready
- For RX: receives data if UART buffer is ready

### Restart (RX mode)
```cpp
int8_t restart();
```
- Resets FIFO and restarts UART reception

## Internal FIFO Management
- `fifo_free_space()`: Returns available space in FIFO
- `fifo_data_length()`: Returns amount of data in FIFO
- `fifo_push(uint8_t c)`: Pushes a byte into FIFO on overrun increases the FIFO tail --> means drops the oldest byte
- `fifo_pop()`: Pops a byte from FIFO

## Usage Example
```cpp
#include "mySerial.h"

mySerial serial5;
bool uart5_TX_ready = true, uart5_RX_ready;

// TX mode initialization
serial5.init(&huart2, &uart_TX_ready, &uart5_RX_ready, 256, 4);



// Write data (TX)
uint8_t tx_data[] = "Hello, UART!";
serial5.write(tx_data, sizeof(tx_data) - 1);

// Read data (RX)
uint8_t rx_data[16];
size_t bytes_read = serial5.read(rx_data, sizeof(rx_data));

// In main loop - optional 
serialTX.send(); // the TX Fifo is auto-flushed if TX_callBackPull(void) is called in HAL_UART_TxCpltCallback

```

---
