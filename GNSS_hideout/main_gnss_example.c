/**
 * @file main_gnss_example.c
 * @brief Complete example of GNSS integration in STM32 main.c
 * 
 * This file shows how to integrate the GNSS wrapper into your existing main.c
 * Copy relevant sections into your actual main.c file.
 * 
 * NOTE: This is an EXAMPLE file. Do NOT add it directly to your build,
 * but use it as a reference for your actual main.c modifications.
 */

// ============================================================================
// INCLUDES - Add these to your existing includes
// ============================================================================

#include "main.h"
#include <stdio.h>
#include "ublox_gnss_example.h"  // NEW: Add this include


// ============================================================================
// GNSS MODULE STATE - Add these as global variables
// ============================================================================

// Flag to track GNSS initialization
static bool gnss_initialized = false;

// Timing for debug output
static uint32_t lastGnssDebugTime = 0;
static const uint32_t GNSS_DEBUG_INTERVAL_MS = 1000;  // Print every 1 second


// ============================================================================
// MAIN FUNCTION - Add GNSS initialization and update calls
// ============================================================================

int main(void) {
    //=========================================================================
    // EXISTING HAL INITIALIZATION (unchanged)
    //=========================================================================
    
    HAL_Init();
    SystemClock_Config();
    
    // Initialize peripheral interfaces
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_UART1_Init();    // Debug serial output
    MX_UART2_Init();    // Your other UART if needed
    MX_UART3_Init();    // GNSS module interface
    MX_TIM1_Init();     // Timers if needed
    MX_TIM2_Init();
    // ... other peripheral initialization ...
    
    // Retarget printf to UART1 (if not already done)
    // setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("\n=== System Starting ===\n");
    
    //=========================================================================
    // NEW: GNSS MODULE INITIALIZATION
    //=========================================================================
    
    printf("Initializing GNSS module...\n");
    
    // Initialize GNSS - this call blocks for up to 2 seconds
    gnss_initialized = gnss_init(&huart3, &huart3_IT_ready);
    
    if (gnss_initialized) {
        printf("GNSS module initialized successfully\n");
    } else {
        printf("ERROR: GNSS module failed to initialize\n");
        printf("Check UART3 configuration and GNSS module connection\n");
        // Optionally retry or continue without GNSS
    }
    
    printf("System initialization complete\n\n");
    
    //=========================================================================
    // MAIN LOOP
    //=========================================================================
    
    while (1) {
        //=====================================================================
        // CRITICAL: GNSS UPDATE - MUST be called frequently!
        //=====================================================================
        
        // Non-blocking update of GNSS data
        // This polls the UART for new data and caches values
        // Call this at least 10Hz (every 100ms), 100Hz (every 10ms) is better
        if (gnss_initialized) {
            gnss_update();
        }
        
        //=====================================================================
        // EXAMPLE: Process GNSS data periodically
        //=====================================================================
        
        if (gnss_initialized && (HAL_GetTick() - lastGnssDebugTime) >= GNSS_DEBUG_INTERVAL_MS) {
            lastGnssDebugTime = HAL_GetTick();
            
            if (gnss_has_valid_fix()) {
                // Extract position data (all non-blocking)
                int32_t latitude  = gnss_get_latitude();
                int32_t longitude = gnss_get_longitude();
                int32_t altitude  = gnss_get_altitude_msl();
                uint8_t siv       = gnss_get_siv();
                int32_t speed     = gnss_get_ground_speed();
                int32_t heading   = gnss_get_heading();
                
                // Format and print position
                // Note: coordinates are in degrees * 10^-7
                // Convert by dividing by 10^7 for decimal degrees
                printf("=== GNSS Position ===\n");
                printf("Lat:  %ld.%07ld°\n", 
                       latitude / 10000000, 
                       (latitude % 10000000) < 0 ? -(latitude % 10000000) : (latitude % 10000000));
                printf("Lon:  %ld.%07ld°\n", 
                       longitude / 10000000, 
                       (longitude % 10000000) < 0 ? -(longitude % 10000000) : (longitude % 10000000));
                printf("Alt:  %ld.%03ld m\n", 
                       altitude / 1000, 
                       altitude % 1000);
                printf("Speed: %ld.%03ld m/s\n", 
                       speed / 1000, 
                       speed % 1000);
                printf("Head:  %ld.%05ld°\n", 
                       heading / 100000, 
                       heading % 100000);
                printf("SIV:   %d\n", siv);
                printf("\n");
                
            } else {
                // Still waiting for fix
                static int waiting_counter = 0;
                printf("Waiting for GNSS fix... (%d seconds)\n", waiting_counter++);
            }
        }
        
        //=====================================================================
        // YOUR OTHER APPLICATION CODE HERE
        //=====================================================================
        
        // Example: Toggle LED to show system is running
        // HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
        // HAL_Delay(500);
        
        // Or use a timer-based main loop instead
        // etc...
    }
    
    return 0;
}


// ============================================================================
// OPTIONAL: Timer-based Main Loop Alternative
// ============================================================================

/**
 * If you want to use a timer interrupt for precise timing instead of the
 * while(1) loop above, use this approach:
 * 
 * In STM32CubeMX:
 *   1. Configure a timer (e.g., TIM2) with 10ms interrupt (100Hz)
 *   2. Enable it in main before the while(1) loop
 *   3. HAL_TIM_Base_Start_IT(&htim2);
 * 
 * Then implement this callback in main.c:
 */

// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//     if (htim->Instance == TIM2) {
//         // Called every 10ms (100Hz)
//         if (gnss_initialized) {
//             gnss_update();  // Non-blocking GNSS update
//         }
//         
//         // Your other periodic tasks here
//     }
// }


// ============================================================================
// DATA CONVERSION HELPERS
// ============================================================================

/**
 * Convert GNSS latitude/longitude to decimal degrees
 * Input: degrees * 10^-7
 * Output: decimal degrees
 */
float gnss_coord_to_decimal(int32_t coord_raw) {
    return coord_raw / 10000000.0f;
}

/**
 * Convert GNSS speed from mm/s to m/s
 */
float gnss_speed_to_ms(int32_t speed_mm_s) {
    return speed_mm_s / 1000.0f;
}

/**
 * Convert GNSS heading from degrees * 10^-5 to degrees
 */
float gnss_heading_to_degrees(int32_t heading_raw) {
    return heading_raw / 100000.0f;
}

/**
 * Convert GNSS altitude from mm to m
 */
float gnss_altitude_to_m(int32_t altitude_mm) {
    return altitude_mm / 1000.0f;
}


// ============================================================================
// DEBUGGING FUNCTIONS
// ============================================================================

/**
 * Call this function if you want detailed GNSS debug information
 */
void gnss_print_raw_data(void) {
    printf("=== Raw GNSS Data ===\n");
    printf("Latitude (raw):  %ld\n", gnss_get_latitude());
    printf("Longitude (raw): %ld\n", gnss_get_longitude());
    printf("Altitude (mm):   %ld\n", gnss_get_altitude_msl());
    printf("Speed (mm/s):    %ld\n", gnss_get_ground_speed());
    printf("Heading (raw):   %ld\n", gnss_get_heading());
    printf("SIV:             %d\n", gnss_get_siv());
    printf("Valid fix:       %s\n", gnss_has_valid_fix() ? "YES" : "NO");
    printf("\n");
}

/**
 * Recommended interval for main loop iteration
 * Call gnss_update() at least this frequently
 */
#define GNSS_UPDATE_INTERVAL_MS 10  // 100Hz update rate
#define GNSS_UPDATE_INTERVAL_MS_MIN 100  // Minimum safe interval (10Hz)

// If your main loop can't keep up at 100Hz, use 10Hz minimum:
// while (1) {
//     if ((millis() - lastUpdate) >= GNSS_UPDATE_INTERVAL_MS_MIN) {
//         gnss_update();
//         lastUpdate = millis();
//     }
// }


// ============================================================================
// COMMON INTEGRATION ISSUES AND SOLUTIONS
// ============================================================================

/**
 * ISSUE: "GNSS initialization failed"
 * SOLUTIONS:
 * 1. Check STM32CubeMX configuration - UART3 must be enabled
 * 2. Check baud rate matches GNSS module (usually 38400)
 * 3. Check UART3_RX interrupt is enabled
 * 4. Verify UART3 is properly initialized by STM32CubeMX
 * 5. Check UART3_RX pin is not being used by other peripherals
 * 
 * ISSUE: "No fix after 1 minute"
 * SOLUTIONS:
 * 1. Antenna must be outdoors with view of sky
 * 2. Cold start (no ephemeris) takes 30-60 seconds
 * 3. Warm start (with ephemeris) takes 5-15 seconds
 * 4. Check antenna connection and quality
 * 5. Try reflashing with fresh firmware
 * 
 * ISSUE: "Data not updating"
 * SOLUTIONS:
 * 1. Ensure gnss_update() is called frequently (every 10-100ms)
 * 2. Check debug output shows SIV > 0
 * 3. Monitor UART3 RX line with logic analyzer
 * 4. Verify UART3 interrupt is firing
 * 5. Check UART3 receive buffer isn't overflowing
 * 
 * ISSUE: "Build errors with undefined symbols"
 * SOLUTIONS:
 * 1. Ensure all .cpp files are compiled (not just .c)
 * 2. Check include paths include Core/Inc
 * 3. Verify SparkFun library header modifications
 * 4. Make sure stm32_arduino_compatibility.h is found
 */

