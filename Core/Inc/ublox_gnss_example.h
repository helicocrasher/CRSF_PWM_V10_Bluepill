/**
 * @file ublox_gnss_example.h
 * @brief Public API for SparkFun u-blox GNSS integration with STM32 HAL
 * 
 * INTEGRATION INSTRUCTIONS:
 * ========================
 * 
 * 1. In STM32CubeMX:
 *    - Configure UART3 with appropriate baud rate (typically 38400 or 115200)
 *    - Enable RX interrupt
 *    - Generate code
 * 
 * 2. In main.c (or user_main.cpp if using this project's structure):
 *    - Call gnss_init() in main setup
 *    - Call gnss_update() periodically (suggested: every 10-100ms in main loop)
 *    - Use gnss_get_*() functions to read data
 * 
 * 3. Add these files to your project:
 *    - Core/Inc/stm32_arduino_compatibility.h
 *    - Core/Src/stm32_arduino_compatibility.cpp
 *    - Core/Inc/ublox_gnss_wrapper.h
 *    - Core/Src/ublox_gnss_wrapper.cpp
 *    - Core/Inc/ublox_gnss_example.h
 *    - Core/Src/ublox_gnss_example.cpp
 *    - SparkFun_u-blox_GNSS_Arduino_Library source files (modified)
 * 
 * USAGE EXAMPLE:
 * ==============
 * 
 * void main(void) {
 *     // STM32 HAL initialization (from STM32CubeMX)
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *     MX_UART1_Init();    // Debug/logging
 *     MX_UART3_Init();    // GNSS module
 *     
 *     // Initialize GNSS
 *     if (!gnss_init(&huart3, &huart3_IT_ready)) {
 *         printf("GNSS initialization failed\n");
 *     }
 *     
 *     // Main loop
 *     while (1) {
 *         gnss_update();  // Call frequently (10-100Hz)
 *         
 *         if (gnss_has_valid_fix()) {
 *             int32_t lat = gnss_get_latitude();
 *             int32_t lon = gnss_get_longitude();
 *             uint8_t siv = gnss_get_siv();
 *             printf("Lat: %ld, Lon: %ld, SIV: %d\n", lat, lon, siv);
 *         }
 *     }
 * }
 */

#ifndef UBLOX_GNSS_EXAMPLE_H
#define UBLOX_GNSS_EXAMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize GNSS module (BLOCKING, ~1-2 seconds)
 * Call once during setup, after UART3 is initialized
 * 
 * @param huart3: STM32 UART3 HAL handle
 * @param huart_IT_ready: Pointer to interrupt ready flag
 * @return true if successful, false on initialization error
 */
bool gnss_init(UART_HandleTypeDef *huart3, bool *huart_IT_ready);

/**
 * @brief Update GNSS data (NON-BLOCKING)
 * Call periodically from main loop (recommended: 10-100Hz, i.e., every 10-100ms)
 */
void gnss_update(void);

/**
 * @brief Get cached latitude (NON-BLOCKING)
 * @return Latitude in degrees * 10^-7 (e.g., 40.123456° = 401234560)
 */
int32_t gnss_get_latitude(void);

/**
 * @brief Get cached longitude (NON-BLOCKING)
 * @return Longitude in degrees * 10^-7
 */
int32_t gnss_get_longitude(void);

/**
 * @brief Get cached altitude above MSL (NON-BLOCKING)
 * @return Altitude in millimeters
 */
int32_t gnss_get_altitude_msl(void);

/**
 * @brief Get cached ground speed (NON-BLOCKING)
 * @return Speed in mm/s
 */
int32_t gnss_get_ground_speed(void);

/**
 * @brief Get cached heading (NON-BLOCKING)
 * @return Heading in degrees * 10^-5 (e.g., 120.345° = 12034500)
 */
int32_t gnss_get_heading(void);

/**
 * @brief Get number of satellites in use (NON-BLOCKING)
 * @return Number of satellites (0 = no fix)
 */
uint8_t gnss_get_siv(void);

/**
 * @brief Check if GNSS has valid fix (NON-BLOCKING)
 * @return true if satellites in use > 0, false if no fix
 */
bool gnss_has_valid_fix(void);

#ifdef __cplusplus
}

// C++ API for advanced users
class SFE_UBLOX_GNSS;
SFE_UBLOX_GNSS& gnss_get_raw(void);

#endif // __cplusplus

#endif // UBLOX_GNSS_EXAMPLE_H
