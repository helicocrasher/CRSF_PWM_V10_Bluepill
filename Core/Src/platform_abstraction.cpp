/*// Arduino-style map() function implementation (for C++ linkage)
#ifdef __cplusplus
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif
*/
/*
 * platform_abstraction.cpp
 * STM32 implementation of Stream abstraction wrapping mySerial
 */

#include "platform_abstraction.h"
#include "mySerial.h"
#include "uart_config.h"
#include "stm32f103xb.h"
#include <cstdint>
#include <cstring>
#include <sys/_intsup.h>


// Global pointer for UART1 receive callback
STM32Stream* g_uartStream = nullptr;

extern UART_HandleTypeDef huart1, huart2, huart3;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern volatile uint32_t RX1_overrun, ELRS_TX_count;
extern volatile uint32_t adcValue, ADC_count;
extern volatile uint8_t isADCFinished;
extern volatile uint8_t i2cWriteComplete;
extern mySerial serialDebug;
extern mySerial serialCrsf;
extern mySerial serialGnss;


// redirection of printf() output to debug UART write()
  
extern "C" int __io_putchar(int ch){
  uint8_t single_char = (uint8_t) ch;
#if UART_ROLE_DEBUG != UART_ROLE_NONE
    serialDebug.write((uint8_t*) &single_char,1 );
#endif
  return single_char;
}


extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        // Handle ADC conversion complete event
        // For example, read the converted value
        // adcValue = HAL_ADC_GetValue(&hadc1);
        // Process adcValue as needed
        ADC_count++;
        isADCFinished = +1;
    }
//    while (1);
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

#if UART_ROLE_CRSF != UART_ROLE_NONE
    if (huart->Instance == UART_CRSF_INSTANCE) {
        serialCrsf.set_ready_TX();
    }
#endif
#if UART_ROLE_DEBUG != UART_ROLE_NONE
    if (huart->Instance == UART_DEBUG_INSTANCE) {
        if (serialDebug.TX_callBackPull() == 0) { // get more data to send if available in FIFO
            serialDebug.set_ready_TX(); // No more data to send, mark debug UART as ready
        }
    }
#endif
#if UART_ROLE_GNSS != UART_ROLE_NONE
    if (huart->Instance == UART_GNSS_INSTANCE) {
        if (serialGnss.TX_callBackPull() == 0) { // get more data to send if available in FIFO
            serialGnss.set_ready_TX();
        }
    }
#endif
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if UART_ROLE_CRSF != UART_ROLE_NONE
    if (huart == UART_CRSF_HANDLE) {
        serialCrsf.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        serialCrsf.receive();
    }
#endif
#if UART_ROLE_DEBUG != UART_ROLE_NONE
    if (huart == UART_DEBUG_HANDLE) {
        serialDebug.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        serialDebug.receive();
    }
#endif
#if UART_ROLE_GNSS != UART_ROLE_NONE
    if (huart == UART_GNSS_HANDLE) {
        serialGnss.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        serialGnss.receive();
    }
#endif
}

// I2C MasterTxCpltCallback
extern "C" void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == &hi2c1) {
        // Handle I2C transmission complete event
        i2cWriteComplete = 1;
    }
}


STM32Stream::STM32Stream(mySerial *serial) 
    : _serial(serial) 
{
    // Nothing to do - mySerial is already initialized
}

int STM32Stream::restartUARTRX(UART_HandleTypeDef *huart) {
    if (_serial) {
        return _serial->restart_RX();  // Use mySerial's restart method
    }
    return -1;
}

int STM32Stream::available() {
    if (_serial) {
        return _serial->available();  // Delegate to mySerial
    }
    return 0;
}

int STM32Stream::read() {
    if (_serial && _serial->available() > 0) {
        uint8_t byte = 0;
        if (_serial->read(&byte, 1) > 0) {
            return byte;
        }
    }
    return -1;  // No data available
}

size_t STM32Stream::write(uint8_t b) {
    if (_serial) {
        return _serial->write(&b, 1);  // Delegate to mySerial
    }
    return 0;
}

size_t STM32Stream::write(const uint8_t *buf, size_t len) {
    if (_serial) {
        return _serial->write(buf, len);  // Delegate to mySerial
    }
    return 0;
}


