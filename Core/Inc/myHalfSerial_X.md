# myHalfSerial_X Class for STM32 HAL

## Overview
`myHalfSerial_X` is a flexible, non-blocking UART communication class for STM32 microcontrollers using the HAL library. It supports both transmission (TX) and reception (RX) modes, managed via interrupt-driven methods. The class uses an internal FIFO buffer for data queuing and provides simple APIs for reading, writing, and updating serial communication.

## Features
- Supports both TX and RX modes (selectable at initialization)
- Non-blocking UART communication using HAL interrupts
- Internal FIFO buffer for data management
- Configurable FIFO and UART buffer sizes
- Simple API for writing, reading, and updating serial state
- Prevents UART buffer overrun (for this in RX mode, the method updateSerial() must be called frequently enough)
- Manages the UART ready state
- In TX mode the FIFO will not overrun 
- In RX mode the FIFO might overrun if data is not read fast enough in that case the oldest data will be over written

## API

### Constructor / Destructor
```cpp
myHalfSerial_X();
~myHalfSerial_X();
```

### Initialization
```cpp
void init(UART_HandleTypeDef *huart, bool *huart_IT_ready, bool isTX = true, size_t fifo_buffer_size = 256, size_t UART_buffer_size = 4);
```
- `huart`: Pointer to the UART handle
- `huart_IT_ready`: Pointer to a flag indicating UART ready state (should be set in UART callbacks)
- `isTX`: Set to `true` for TX mode, `false` for RX mode
- `fifo_buffer_size`: Size of the internal FIFO buffer (default 256)
- `UART_buffer_size`: Size of the temporary UART buffer (default 4)

### Write Data (TX mode)
```cpp
size_t write(const uint8_t *input_array, size_t len);
```
- Copies up to `len` bytes from `input_array` into the FIFO buffer
- Returns the number of bytes actually written (may be less if FIFO is full)
- NEW: if no transmission is ongoing (huart_IT_ready) the internal sen() method is called 
-->self start.

### TX_callBackPull NEW!
```cpp
size_t TX_callBackPull(void)
```
- request for the next HAL_UART_Transmit_IT
- if data is in the FIFO a next transmission is started
- return value the number of bytes put to transmission max UART_buffersize, min 0
- (the HAL_UART_TxCpltCallback must set the huart_IT_ready after TX_callBackPull==0)

### Read Data (RX mode)
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
#include "myHalfSerial_X.h"

myHalfSerial_X serialTX, SerialRX;
bool uart_ready = true;

// TX mode initialization
serialTX.init(&huart2, &uart_ready, true, 256, 8);

// RX mode initialization
serialRX.init(&huart2, &uart_ready, false, 256, 8);

// Write data (TX)
uint8_t tx_data[] = "Hello, UART!";
serialTX.write(tx_data, sizeof(tx_data) - 1);

// Read data (RX)
uint8_t rx_data[16];
size_t bytes_read = serialRX.read(rx_data, sizeof(rx_data));

// In main loop
serialTX.updateSerial();
serialRX.updateSerial();
```

---
