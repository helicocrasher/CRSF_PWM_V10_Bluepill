/**
 * @file stm32_arduino_compatibility.cpp
 * @brief Implementation of Arduino compatibility layer for STM32 HAL
 */

#include "stm32_arduino_compatibility.h"
#include "mySerial.h"

// ============================================================================
// Global Variables
// ============================================================================

STM32Serial *pSerial = nullptr;
TwoWire Wire;
SPIClass SPI;

// Global serial instance for UART2 debug - initialized by Serial_InitUART2()
static STM32Serial *g_Serial = nullptr;

// System timing for millis/micros
static uint32_t g_microsPerTick = 0;
static bool g_timing_initialized = false;

// ============================================================================
// Timing Functions Implementation
// ============================================================================

static void timing_init(void) {
    if (g_timing_initialized) return;
    
    // Calculate microseconds per SysTick
    // SysTick frequency is typically 1kHz (1ms period)
    // This is: 1000000 us / 1000 Hz = 1000 us/tick
    uint32_t systick_freq = 1000; // Default 1kHz
    g_microsPerTick = 1000000 / systick_freq;
    
    g_timing_initialized = true;
}

uint32_t millis(void) {
    timing_init();
    return HAL_GetTick();
}

uint32_t micros(void) {
    timing_init();
    
    uint32_t ticks = HAL_GetTick();
    uint32_t counter = SysTick->VAL;
    
    // SysTick counts DOWN, so we need to invert
    uint32_t load = SysTick->LOAD;
    uint32_t elapsed_in_tick = (load - counter) * g_microsPerTick / load;
    
    return (ticks * 1000000) + elapsed_in_tick;
}

void delay(uint32_t ms) {
    HAL_Delay(ms);
}

void delayMicroseconds(uint32_t us) {
    uint32_t start = micros();
    while ((micros() - start) < us) {
        // Busy wait
    }
}

// ============================================================================
// STM32Serial Implementation
// ============================================================================

STM32Serial::STM32Serial(mySerial *port) : serialPort(port) {
}

size_t STM32Serial::write(uint8_t c) {
    if (!serialPort) return 0;
    return serialPort->write(&c, 1);
}

size_t STM32Serial::write(const uint8_t *buffer, size_t size) {
    if (!serialPort) return 0;
    return serialPort->write(buffer, size);
}

int STM32Serial::read(void) {
    if (!serialPort) return -1;
    uint8_t byte;
    size_t read_count = serialPort->read(&byte, 1);
    return (read_count > 0) ? (int)byte : -1;
}

int STM32Serial::available(void) {
    if (!serialPort) return 0;
    return serialPort->available();
}

void STM32Serial::flush(void) {
    if (!serialPort) 
    while (serialPort->send() > 0) {
        // Wait until all data is sent
    }
return;
    // mySerial is non-blocking, if there is need for a blocking flush, that should do the job
}

// ============================================================================
// Serial Initialization for UART2
// ============================================================================

// Global UART2 debug instance
static mySerial g_debug_uart2_instance;
static STM32Serial g_Serial_instance(&g_debug_uart2_instance);
STM32Serial *Serial_ptr = nullptr;  // Will be initialized by Serial_InitUART2

void Serial_InitUART2(void) {
    extern UART_HandleTypeDef huart2;
    extern volatile bool ready_TX_UART2,ready_RX_UART2;
    
    // Initialize the UART2 instance with UART2 handle
    g_debug_uart2_instance.init(&huart2, (bool*)&ready_TX_UART2, (bool*)&ready_RX_UART2, 256, 4);
    
    // Make it available globally via both pointers
    g_Serial = &g_Serial_instance;
    Serial_ptr = &g_Serial_instance;
}

// Get pointer to the global Serial instance
STM32Serial* GetSerialRef(void) {
    if (!Serial_ptr) {
        Serial_InitUART2();
    }
    return Serial_ptr;
}

// ============================================================================
// GPIO Stub Implementation
// ============================================================================

void pinMode(uint8_t pin, uint8_t mode) {
    // Stub: no operation for STM32 HAL (GPIO setup is done in STM32CubeMX/HAL init)
}

void digitalWrite(uint8_t pin, uint8_t val) {
    // Stub: GPIO control would be done via HAL_GPIO_WritePin if needed
    // For now, this is a no-op
}

int digitalRead(uint8_t pin) {
    // Stub: would use HAL_GPIO_ReadPin if needed
    return 0;
}
