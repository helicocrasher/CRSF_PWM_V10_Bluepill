/**
 * @file stm32_arduino_compatibility.h
 * @brief Arduino compatibility layer for STM32 HAL projects
 * 
 * This file provides Arduino-like abstractions to allow Arduino libraries to work
 * with STM32 HAL without the Arduino framework. It includes:
 * - Basic type definitions
 * - Stream class for serial communication
 * - Stub classes for Wire (I2C) and SPI
 * - Timing functions (millis, micros, delay)
 */

#ifndef STM32_ARDUINO_COMPATIBILITY_H
#define STM32_ARDUINO_COMPATIBILITY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <cstdarg>
#include "stm32f1xx_hal.h"

// ============================================================================
// Type Definitions
// ============================================================================

typedef uint8_t byte;
typedef unsigned short word;

// ============================================================================
// Arduino Print Formatting Constants
// ============================================================================

#define HEX 16
#define DEC 10
#define OCT 8
#define BIN 2
#define MSBFIRST 1
#define LSBFIRST 0
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

// F() macro for storing strings in flash memory (not applicable on STM32, just returns the string)
#define F(str) (str)

// ============================================================================
// Timing Functions
// ============================================================================

/**
 * @brief Get milliseconds since system start
 * Uses HAL_GetTick() internally
 */
uint32_t millis(void);

/**
 * @brief Get microseconds since system start
 * Derived from core clock and HAL timer
 */
uint32_t micros(void);

/**
 * @brief Blocking delay in milliseconds
 */
void delay(uint32_t ms);

/**
 * @brief Blocking delay in microseconds
 */
void delayMicroseconds(uint32_t us);

#ifdef __cplusplus
// ============================================================================
// Stream Class (Abstract)
// ============================================================================

class Stream {
public:
    virtual ~Stream() {}
    
    /**
     * @brief Write a single byte
     * @return Number of bytes written (1 or 0)
     */
    virtual size_t write(uint8_t c) = 0;
    
    /**
     * @brief Write multiple bytes
     * @return Number of bytes written
     */
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    
    /**
     * @brief Read a single byte (non-blocking)
     * @return Byte value (0-255) or -1 if no data available
     */
    virtual int read(void) = 0;
    
    /**
     * @brief Check how many bytes are available to read
     */
    virtual int available(void) = 0;
    
    /**
     * @brief Peek at next byte without consuming it
     */
    virtual int peek(void) { return -1; }
    
    /**
     * @brief Flush output buffer
     */
    virtual void flush(void) {}
    
    /**
     * @brief Read data into buffer (non-blocking)
     */
    virtual size_t readBytes(uint8_t *buffer, size_t length) {
        size_t count = 0;
        unsigned long startMicros = micros();
        while (count < length) {
            int c = read();
            if (c < 0) {
                if (micros() - startMicros > 1000000) break; // 1 second timeout
                continue;
            }
            *buffer++ = (uint8_t)c;
            count++;
        }
        return count;
    }
    
    /**
     * @brief Print text without newline
     */
    size_t print(const char *str) {
        if (!str) return 0;
        return write((const uint8_t *)str, strlen(str));
    }
    
    /**
     * @brief Print text with newline
     */
    size_t println(const char *str) {
        size_t n = 0;
        if (str) n = write((const uint8_t *)str, strlen(str));
        write((uint8_t)'\r');
        n += write((uint8_t)'\n');
        return n;
    }
    
    /**
     * @brief Print empty line
     */
    size_t println(void) {
        size_t n = write((uint8_t)'\r');
        n += write((uint8_t)'\n');
        return n;
    }
    
    /**
     * @brief Print an integer with optional base
     */
    size_t print(long n, int base = DEC) {
        char buffer[33];
        if (base == HEX) {
            snprintf(buffer, sizeof(buffer), "%lx", n);
        } else if (base == OCT) {
            snprintf(buffer, sizeof(buffer), "%lo", n);
        } else if (base == BIN) {
            // Handle binary separately
            int idx = 0;
            unsigned long val = (n < 0) ? -n : n;
            if (n < 0) buffer[idx++] = '-';
            if (val == 0) buffer[idx++] = '0';
            else {
                char bits[32];
                int bitIdx = 0;
                while (val) {
                    bits[bitIdx++] = (val & 1) ? '1' : '0';
                    val >>= 1;
                }
                for (int i = bitIdx - 1; i >= 0; i--) buffer[idx++] = bits[i];
            }
            buffer[idx] = 0;
        } else {
            snprintf(buffer, sizeof(buffer), "%ld", n);
        }
        return write((const uint8_t *)buffer, strlen(buffer));
    }
    
    /**
     * @brief Print an integer with newline
     */
    size_t println(long n, int base = DEC) {
        size_t len = print(n, base);
        len += write((uint8_t)'\r');
        len += write((uint8_t)'\n');
        return len;
    }
    
    /**
     * @brief Print an unsigned integer
     */
    size_t print(unsigned long n, int base = DEC) {
        return print((long)n, base);
    }
    
    /**
     * @brief Print a double
     */
    size_t print(double n, int digits = 2) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.*f", digits, n);
        return write((const uint8_t *)buffer, strlen(buffer));
    }
    
    /**
     * @brief Print integer variants
     */
    size_t print(int n, int base = DEC) { return print((long)n, base); }
    size_t println(int n, int base = DEC) { return println((long)n, base); }
    size_t print(unsigned int n, int base = DEC) { return print((long)n, base); }
    size_t println(unsigned int n, int base = DEC) { return println((long)n, base); }
};

// ============================================================================
// Serial (UART) Stream Implementation
// ============================================================================

class STM32Serial : public Stream {
private:
    class myHalfSerial_X *serialPort;
    
public:
    STM32Serial(class myHalfSerial_X *port);
    virtual ~STM32Serial() {}
    
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    int read(void) override;
    int available(void) override;
    void flush(void) override;
};

// Global serial instances
extern STM32Serial *pSerial;
extern STM32Serial *Serial_ptr;  // Global Serial object for UART2 debug (pointer)

/**
 * @brief Initialize the Serial instance for UART2 debug output
 * Call this once during initialization before using Serial
 */
void Serial_InitUART2(void);

/**
 * @brief Get pointer to the global Serial instance for UART2 debug
 * Used for compatibility with Arduino code that expects a Serial object
 */
extern STM32Serial* GetSerialRef(void);

// Convenience macro to use Serial like the Arduino version
#define Serial (*GetSerialRef())

// ============================================================================
// Wire (I2C) Stub Class
// ============================================================================

class TwoWire {
public:
    TwoWire() {}
    virtual ~TwoWire() {}
    
    // Stub methods - I2C not supported, only serial
    void begin(uint8_t address = 0) {}
    void begin(int sda, int scl) {}
    void end() {}
    void setClock(uint32_t freq) {}
    void setClockStretchLimit(uint32_t limit) {}
    
    // Not implemented for STM32 UART-only operation
    uint8_t requestFrom(uint8_t address, uint8_t quantity) { return 0; }
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) { return 0; }
    void beginTransmission(uint8_t address) {}
    uint8_t endTransmission(void) { return 4; }
    uint8_t endTransmission(uint8_t sendStop) { return 4; }
    size_t write(uint8_t data) { return 0; }
    size_t write(const uint8_t *data, size_t quantity) { return 0; }
    int read(void) { return -1; }
    int available(void) { return 0; }
    int peek(void) { return -1; }
    void flush(void) {}
};

extern TwoWire Wire;

// ============================================================================
// SPI Stub Class
// ============================================================================

class SPISettings {
public:
    SPISettings(uint32_t clock = 1000000, uint8_t bitOrder = 0, uint8_t dataMode = 0) {}
};

class SPIClass {
public:
    SPIClass() {}
    virtual ~SPIClass() {}
    
    void begin(void) {}
    void end(void) {}
    void beginTransaction(SPISettings settings) {}
    void endTransaction(void) {}
    uint8_t transfer(uint8_t data) { return 0; }
    uint16_t transfer16(uint16_t data) { return 0; }
    void transfer(void *buf, size_t count) {}
    void setDataMode(uint8_t mode) {}
    void setClockDivider(uint8_t div) {}
    void setBitOrder(uint8_t bitOrder) {}
};

extern SPIClass SPI;

// ============================================================================
// GPIO Stub (for debug pins)
// ============================================================================

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

// ============================================================================
// Arduino String Class (Basic Implementation)
// ============================================================================

class String {
private:
    char *buffer;
    size_t capacity;
    size_t len;
    
    void reserve(size_t newCapacity) {
        if (newCapacity <= capacity) return;
        char *newBuffer = (char *)malloc(newCapacity);
        if (!newBuffer) return;
        if (buffer) {
            if (len > 0) memcpy(newBuffer, buffer, len);
            free(buffer);
        }
        buffer = newBuffer;
        capacity = newCapacity;
        if (len == 0) buffer[0] = 0;
    }
    
public:
    String() : buffer(nullptr), capacity(0), len(0) {
        reserve(16);
        if (buffer) buffer[0] = 0;
    }
    
    String(const char *str) : buffer(nullptr), capacity(0), len(0) {
        if (!str) {
            reserve(16);
            if (buffer) buffer[0] = 0;
        } else {
            len = strlen(str);
            reserve(len + 1);
            if (buffer) {
                memcpy(buffer, str, len);
                buffer[len] = 0;
            }
        }
    }
    
    String(const String &str) : buffer(nullptr), capacity(0), len(0) {
        len = str.len;
        reserve(len + 1);
        if (buffer && str.buffer) {
            memcpy(buffer, str.buffer, len);
            buffer[len] = 0;
        }
    }
    
    ~String() {
        if (buffer) free(buffer);
    }
    
    String& operator=(const char *str) {
        if (!str) {
            len = 0;
            if (buffer) buffer[0] = 0;
        } else {
            len = strlen(str);
            reserve(len + 1);
            if (buffer) {
                memcpy(buffer, str, len);
                buffer[len] = 0;
            }
        }
        return *this;
    }
    
    String& operator=(const String &str) {
        if (this != &str) {
            len = str.len;
            reserve(len + 1);
            if (buffer && str.buffer) {
                memcpy(buffer, str.buffer, len);
                buffer[len] = 0;
            }
        }
        return *this;
    }
    
    String& operator+=(const char *str) {
        if (!str) return *this;
        size_t addLen = strlen(str);
        reserve(len + addLen + 1);
        if (buffer) {
            memcpy(buffer + len, str, addLen);
            len += addLen;
            buffer[len] = 0;
        }
        return *this;
    }
    
    String& operator+=(char c) {
        reserve(len + 2);
        if (buffer) {
            buffer[len++] = c;
            buffer[len] = 0;
        }
        return *this;
    }
    
    String operator+(const char *str) const {
        String result(*this);
        result += str;
        return result;
    }
    
    bool operator==(const char *str) const {
        if (!buffer || !str) return buffer == str;
        return strcmp(buffer, str) == 0;
    }
    
    bool operator!=(const char *str) const {
        return !(*this == str);
    }
    
    const char *c_str() const {
        return buffer ? buffer : "";
    }
    
    char *c_str() {
        return buffer ? buffer : (char *)"";
    }
    
    size_t length() const { return len; }
    size_t size() const { return len; }
    
    void clear() {
        len = 0;
        if (buffer) buffer[0] = 0;
    }
    
    char charAt(size_t index) const {
        if (index >= len) return 0;
        return buffer[index];
    }
    
    char operator[](size_t index) const {
        return charAt(index);
    }
    
    int indexOf(char c, unsigned int fromIndex = 0) const {
        if (!buffer) return -1;
        for (unsigned int i = fromIndex; i < len; i++) {
            if (buffer[i] == c) return i;
        }
        return -1;
    }
    
    bool startsWith(const char *prefix) const {
        if (!buffer || !prefix) return false;
        return strncmp(buffer, prefix, strlen(prefix)) == 0;
    }
    
    String substring(unsigned int beginIndex) const {
        if (beginIndex >= len) return String();
        return String(buffer + beginIndex);
    }
    
    String substring(unsigned int beginIndex, unsigned int endIndex) const {
        if (beginIndex >= len || beginIndex >= endIndex) return String();
        size_t subLen = endIndex - beginIndex;
        if (subLen > len - beginIndex) subLen = len - beginIndex;
        String sub;
        sub.reserve(subLen + 1);
        if (sub.buffer) {
            memcpy(sub.buffer, buffer + beginIndex, subLen);
            sub.buffer[subLen] = 0;
            sub.len = subLen;
        }
        return sub;
    }
};

#endif // __cplusplus

#endif // STM32_ARDUINO_COMPATIBILITY_H
