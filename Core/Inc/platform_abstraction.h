/*
 * platform_abstraction.h
 * Platform abstraction layer for STM32 without Arduino
 * Provides STM32Stream for CRSF library
 */

#ifndef PLATFORM_ABSTRACTION_H
#define PLATFORM_ABSTRACTION_H

#include "main.h"
#include <stdint.h>
#include <string.h>

void Serial2Debug_print(char* msg) ;

void Serial2Debug_println(char*  msg) ;


#ifdef __cplusplus
// Forward declaration
class mySerial;

// C++ only - include stm32_arduino_compatibility for Stream class
#include "stm32_arduino_compatibility.h"

// STM32Stream wrapper around mySerial
// Inherits Stream from stm32_arduino_compatibility.h
// Provides a unified interface for both AlfredoCRSF and ublox_gnss
class STM32Stream : public Stream {
public:
    // Constructor wraps an existing mySerial instance
    STM32Stream(mySerial *serial);
    
    // Reinitialize the underlying mySerial (for RX restart)
    int restartUARTRX(UART_HandleTypeDef *huart);
    
    // Stream interface - delegates to mySerial
    int available() override;
    int read() override;
    size_t write(uint8_t b) override;
    size_t write(const uint8_t *buf, size_t len) override;
    
    // Access to underlying mySerial for advanced operations
    mySerial* getSerial() { return _serial; }

private:
    mySerial *_serial;  // Wrapped mySerial instance
};
#endif
// Only visible for C++

#ifdef __cplusplus
class STM32Stream;
extern STM32Stream* g_uartStream;
#endif

// C-compatible helper to re-arm UART RX interrupt
#ifdef __cplusplus
extern "C" {
#endif
void stm32stream_rearm_rx_irq(void);
#ifdef __cplusplus
}
#endif
/*
// Platform-specific millis() implementation
uint32_t platform_millis();

// Helper macro for compatibility
#define millis() platform_millis()
*/

#ifdef __cplusplus
// Arduino-style map() function
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif

#endif /* PLATFORM_ABSTRACTION_H */
