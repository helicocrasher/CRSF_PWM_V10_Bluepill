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
extern mySerial serial2;
extern mySerial crsfSerialWrapper;
extern mySerial gnssSerialWrapper;

char UART1_TX_Buffer[64];

// redirection of printf() output to serial2.write()
  
extern "C" int __io_putchar(int ch){
  uint8_t single_char = (uint8_t) ch;
  serial2.write((uint8_t*) &single_char,1 );
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

    if (huart->Instance == USART1) {
        crsfSerialWrapper.set_ready_TX(); 
    }
    if (huart->Instance == USART2) {
        if (serial2.TX_callBackPull()==0) { // get more data to send if available in FIFO ! make sure to lower interrupt Prio 
                                            // otherwise it might disturb other time critical interrupt routines
            serial2.set_ready_TX(); // No more data to send, mark UART2 as ready
        } 
    }
    if (huart->Instance == USART3 ){
        if (gnssSerialWrapper.TX_callBackPull()==0) { // get more data to send if available in FIFO ! make sure to lower interrupt Prio 
//        {   
            gnssSerialWrapper.set_ready_TX();
        }
    }
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1) {
        //ready_RX_UART1 = 1;
        crsfSerialWrapper.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        crsfSerialWrapper.receive();
    }
    if (huart == &huart2) {
        //ready_RX_UART2 = 1;
        serial2.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        serial2.receive();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        serial2.receive();
    }
    if (huart == &huart3) {
        //ready_RX_UART3 = 1;
        gnssSerialWrapper.set_ready_RX();
        // push the received data to this RX FIFO and Re-arm RX reception for next byte
        gnssSerialWrapper.receive();
    }
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


