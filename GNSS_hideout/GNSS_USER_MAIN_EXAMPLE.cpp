/**
 * @file user_main_gnss_example.cpp
 * @brief Complete example code for GNSS integration in user_main.cpp
 * 
 * This file contains complete, ready-to-use code that you can copy-paste
 * or adapt for your user_main.cpp and user_main.h files.
 * 
 * DO NOT add this entire file to your project - use it as a reference
 * and copy the relevant sections to your user_main.h and user_main.cpp
 */

// ============================================================================
// PART 1: ADD TO user_main.h
// ============================================================================

/*
#ifndef USER_MAIN_H
#define USER_MAIN_H

// ... existing includes ...

// ADD THIS LINE:
#include "ublox_gnss_example.h"

// ... existing code ...

#ifdef __cplusplus
extern "C" {
#endif

// ... existing declarations ...

// ADD THESE FUNCTION DECLARATIONS:
void gnss_module_init(void);
void gnss_module_update(uint32_t millis_now);

#ifdef __cplusplus
}
#endif

#endif // USER_MAIN_H
*/

// ============================================================================
// PART 2: ADD TO user_main.cpp
// ============================================================================

/*

// At the top with other includes:
#include "ublox_gnss_example.h"
#include "myHalfSerial_X.h"

// GNSS module state variables
static bool gnss_initialized = false;
static uint32_t gnss_last_update_time = 0;
static const uint32_t GNSS_UPDATE_INTERVAL_MS = 100;  // Update every 100ms (10Hz)
static uint32_t gnss_last_print_time = 0;
static const uint32_t GNSS_PRINT_INTERVAL_MS = 2000;  // Print every 2 seconds


//==========================================================================
// GNSS Module Initialization Function
// Call this ONCE during system startup
//=========================================================================
void gnss_module_init(void) {
    printf("\n>>> Initializing GNSS Module...\n");
    
    // This call blocks for ~1-2 seconds while waiting for module response
    // Safe to call during initialization
    gnss_initialized = gnss_init(&huart3, &huart3_IT_ready);
    
    if (gnss_initialized) {
        printf(">>> GNSS Module: INITIALIZED OK\n");
        printf(">>> Waiting for satellite fix (may take 30-60 seconds on cold start)...\n\n");
        gnss_last_update_time = millis();
        gnss_last_print_time = millis();
    } else {
        printf(">>> GNSS Module: INITIALIZATION FAILED\n");
        printf(">>> Check:\n");
        printf(">>>  1. UART3 is enabled in STM32CubeMX\n");
        printf(">>>  2. Baud rate matches module (38400 or 115200)\n");
        printf(">>>  3. GNSS module is powered and connected\n");
        printf(">>>  4. RX antenna is connected\n\n");
    }
}


//==========================================================================
// GNSS Module Periodic Update - NON-BLOCKING
// Call this frequently from main loop (recommended: every 10-100ms)
// This function is completely non-blocking and safe to call from loop or ISR
//==========================================================================
void gnss_module_update(uint32_t millis_now) {
    // Do nothing if GNSS not initialized
    if (!gnss_initialized) {
        return;
    }
    
    // Update GNSS data at specified interval
    // This reads any available UART data and updates cached position
    if ((millis_now - gnss_last_update_time) >= GNSS_UPDATE_INTERVAL_MS) {
        gnss_update();  // Non-blocking UART poll and data cache
        gnss_last_update_time = millis_now;
    }
    
    // Periodically display position (optional, for debugging)
    if ((millis_now - gnss_last_print_time) >= GNSS_PRINT_INTERVAL_MS) {
        if (gnss_has_valid_fix()) {
            int32_t lat = gnss_get_latitude();
            int32_t lon = gnss_get_longitude();
            int32_t alt = gnss_get_altitude_msl();
            uint8_t siv = gnss_get_siv();
            int32_t speed = gnss_get_ground_speed();
            int32_t heading = gnss_get_heading();
            
            // Format and display position
            // Coordinates are in degrees * 10^-7, convert to decimal degrees
            float lat_deg = lat / 10000000.0f;
            float lon_deg = lon / 10000000.0f;
            float alt_m = alt / 1000.0f;
            float speed_ms = speed / 1000.0f;
            float heading_deg = heading / 100000.0f;
            
            printf("[GNSS] Lat: %.7f, Lon: %.7f, Alt: %.1fm, Speed: %.2fm/s, Heading: %.1f°, SIV: %d\n",
                   lat_deg, lon_deg, alt_m, speed_ms, heading_deg, siv);
        } else {
            // Still waiting for fix
            printf("[GNSS] Waiting for fix... (SIV: %d/4+)\n", gnss_get_siv());
        }
        gnss_last_print_time = millis_now;
    }
}


//==========================================================================
// HELPER: Get current position data (non-blocking)
// Returns: structure containing position data
//==========================================================================
struct GnssData {
    bool valid;
    int32_t latitude;      // degrees * 10^-7
    int32_t longitude;     // degrees * 10^-7
    int32_t altitude_msl;  // millimeters
    int32_t ground_speed;  // mm/s
    int32_t heading;       // degrees * 10^-5
    uint8_t siv;           // satellites in use
};

GnssData gnss_get_position(void) {
    GnssData data = {0};
    data.valid = gnss_has_valid_fix();
    data.latitude = gnss_get_latitude();
    data.longitude = gnss_get_longitude();
    data.altitude_msl = gnss_get_altitude_msl();
    data.ground_speed = gnss_get_ground_speed();
    data.heading = gnss_get_heading();
    data.siv = gnss_get_siv();
    return data;
}


//==========================================================================
// HELPER: Check if GNSS has valid position
//==========================================================================
bool gnss_position_valid(void) {
    return gnss_initialized && gnss_has_valid_fix();
}


//==========================================================================
// HELPER: Get position as floating point degrees
//==========================================================================
void gnss_get_position_float(float* p_lat, float* p_lon) {
    if (p_lat) *p_lat = gnss_get_latitude() / 10000000.0f;
    if (p_lon) *p_lon = gnss_get_longitude() / 10000000.0f;
}


//==========================================================================
// EXAMPLE: Integration into existing user_main loop
// 
// If you have an existing loop structure like:
//     void user_loop(uint32_t millis_now) { ... }
// 
// Simply add these two lines in the loop:
//
//     gnss_module_update(millis_now);  // Non-blocking, call frequently
//
// And call gnss_module_init() once during user setup
//==========================================================================

extern "C" {

// Your existing initialization function
void user_init(void) {
    // ... existing initialization code ...
    
    // NEW: Initialize GNSS module (blocking, safe during init)
    gnss_module_init();
    
    // ... rest of init ...
}

// Your existing main loop
void user_loop(uint32_t millis_now) {
    // ... your existing code ...
    
    // NEW: Update GNSS data (non-blocking, must be called frequently)
    gnss_module_update(millis_now);
    
    // ... rest of loop ...
}

}  // extern "C"
*/

// ============================================================================
// PART 3: HOW TO USE GNSS DATA IN OTHER FUNCTIONS
// ============================================================================

/*
Example 1: Send position to telemetry
──────────────────────────────────────
void send_position_telemetry(void) {
    if (gnss_has_valid_fix()) {
        int32_t lat = gnss_get_latitude();
        int32_t lon = gnss_get_longitude();
        int32_t alt = gnss_get_altitude_msl();
        uint8_t siv = gnss_get_siv();
        
        // Send to telemetry system
        telemetry_send_position(lat, lon, alt, siv);
    }
}


Example 2: Log position to SD card
──────────────────────────────────
void log_position(void) {
    if (gnss_has_valid_fix()) {
        GnssData pos = gnss_get_position();
        
        // Write to SD card
        char buffer[100];
        sprintf(buffer, "%ld,%ld,%ld,%d\n",
                pos.latitude, pos.longitude, pos.altitude_msl, pos.siv);
        
        sd_card_write(buffer);
    }
}


Example 3: Check if moving
──────────────────────────
bool is_moving(void) {
    return gnss_has_valid_fix() && (gnss_get_ground_speed() > 100);  // > 0.1 m/s
}


Example 4: Get current heading
────────────────────────────────
float get_heading_degrees(void) {
    return gnss_get_heading() / 100000.0f;
}
*/

// ============================================================================
// PART 4: INTEGRATION INTO YOUR EXISTING CRSF/BARO/TELEMETRY CODE
// ============================================================================

/*
If you want to send GNSS position in CRSF telemetry:

In your telemetry/CRSF sending code, add something like:

    // Inside your telemetry task
    if (gnss_has_valid_fix()) {
        float lat = gnss_get_latitude() / 10000000.0f;
        float lon = gnss_get_longitude() / 10000000.0f;
        int32_t alt = gnss_get_altitude_msl() / 1000;  // Convert mm to m
        
        // Send CRSF telemetry with position
        crsf_send_gps(lat, lon, alt, gnss_get_ground_speed(), gnss_get_heading());
    }
*/

// ============================================================================
// NOTES
// ============================================================================

/*
✓ gnss_module_init() - BLOCKING, call ONCE during setup (1-2 seconds)
✓ gnss_module_update() - NON-BLOCKING, call FREQUENTLY (10-100 Hz)
✓ gnss_get_*() functions - NON-BLOCKING, returns cached values (< 1μs)

When integrating:
1. Add includes to user_main.h
2. Add function declarations to user_main.h
3. Copy GNSS implementation code to user_main.cpp
4. Call gnss_module_init() once during init
5. Call gnss_module_update() frequently in main loop
6. Use gnss_get_*() functions to access data

No changes needed to main.c at all!
*/
