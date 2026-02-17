/**
 * @file user_main_gnss_integration.h
 * @brief Code snippets for integrating GNSS into user_main.h/user_main.cpp
 * 
 * This file shows how to add GNSS support to your user_main.h and user_main.cpp
 * without modifying main.c
 */

// ============================================================================
// ADD TO user_main.h (in the includes/declarations section)
// ============================================================================

/*
In user_main.h, add these lines after existing includes:

    // GNSS Module integration
    #include "ublox_gnss_example.h"
    
Then add these function declarations in the extern "C" block:

    // GNSS setup and processing
    void gnss_setup(void);
    void gnss_task(uint32_t millis_now);
    
    // GNSS data access
    bool gnss_has_data(void);
    void gnss_print_position(void);
*/

// ============================================================================
// ADD TO user_main.cpp (implementation)
// ============================================================================

/*
1. At the top with other includes:

    #include "ublox_gnss_example.h"
    #include "myHalfSerial_X.h"

2. Add global variables (for GNSS):

    // GNSS Module state and timing
    static bool gnss_ready = false;
    static uint32_t gnss_last_update = 0;
    static const uint32_t GNSS_UPDATE_INTERVAL_MS = 100;  // Update every 100ms
    static uint32_t gnss_last_print = 0;
    static const uint32_t GNSS_PRINT_INTERVAL_MS = 1000;  // Print every 1 second

3. Add initialization function:

    void gnss_setup(void) {
        // Initialize GNSS module - this is BLOCKING for 1-2 seconds
        // Call this once during system startup
        
        printf("Initializing GNSS module...\n");
        
        // Initialize GNSS on UART3 (non-blocking after init completes)
        gnss_ready = gnss_init(&huart3, &huart3_IT_ready);
        
        if (gnss_ready) {
            printf("GNSS module initialized successfully\n");
        } else {
            printf("ERROR: GNSS module initialization failed\n");
            printf("Check UART3 configuration and GNSS module connection\n");
        }
        
        gnss_last_update = millis();
        gnss_last_print = millis();
    }

4. Add periodic update task (call from main loop):

    void gnss_task(uint32_t millis_now) {
        // Non-blocking GNSS update task
        // Call this frequently from main loop (every 10-100ms)
        
        if (!gnss_ready) {
            return;  // GNSS not initialized
        }
        
        // Update GNSS data at specified interval
        if ((millis_now - gnss_last_update) >= GNSS_UPDATE_INTERVAL_MS) {
            gnss_update();  // Non-blocking UART polling and data caching
            gnss_last_update = millis_now;
        }
        
        // Periodically print position data
        if ((millis_now - gnss_last_print) >= GNSS_PRINT_INTERVAL_MS) {
            gnss_print_position();
            gnss_last_print = millis_now;
        }
    }

5. Add helper function for printing:

    void gnss_print_position(void) {
        if (!gnss_ready) {
            return;
        }
        
        if (gnss_has_valid_fix()) {
            int32_t lat = gnss_get_latitude();
            int32_t lon = gnss_get_longitude();
            int32_t alt = gnss_get_altitude_msl();
            uint8_t siv = gnss_get_siv();
            int32_t speed = gnss_get_ground_speed();
            
            printf("GNSS - Lat: %ld.%07ld, Lon: %ld.%07ld, Alt: %ldm, SIV: %d, Speed: %ld.%03ldm/s\n",
                   lat / 10000000, (lat % 10000000),
                   lon / 10000000, (lon % 10000000),
                   alt / 1000,
                   siv,
                   speed / 1000, (speed % 1000));
        } else {
            printf("GNSS - Waiting for fix (SIV: %d)\n", gnss_get_siv());
        }
    }

6. Add data query helper:

    bool gnss_has_data(void) {
        return gnss_ready && gnss_has_valid_fix();
    }
*/

// ============================================================================
// INTEGRATION INTO EXISTING user_main.cpp MAIN LOOP
// ============================================================================

/*
In user_main.cpp, in your existing main loop/periodic task function, add:

    void user_loop(uint32_t millis_now) {
        // ... your existing code ...
        
        // NEW: Call GNSS task
        gnss_task(millis_now);
        
        // ... rest of your code ...
    }

Or if you don't have a centralized loop, add gnss_task() calls wherever you have 
periodic task execution (main loop, timer callback, etc.)
*/

// ============================================================================
// INTEGRATION FLOW IN user_main.cpp
// ============================================================================

/*
Typical initialization flow (in C section of user_main.cpp):

    extern "C" {
        void user_init(void) {
            // ... existing initialization code ...
            
            // Initialize GNSS module (blocking, ~1-2 seconds)
            gnss_setup();
            
            // ... other initializations ...
        }
        
        void user_loop(void) {
            static uint32_t loop_timer = 0;
            uint32_t now = millis_get_value();  // or your timing function
            
            // ... existing code ...
            
            // Non-blocking GNSS update (call frequently)
            gnss_task(now);
            
            // ... rest of loop ...
        }
    }
*/

// ============================================================================
// DATA ACCESS EXAMPLES
// ============================================================================

/*
In any function in user_main.cpp where you need GNSS data:

    // Check if we have valid position
    if (gnss_has_valid_fix()) {
        int32_t latitude = gnss_get_latitude();        // degrees * 10^-7
        int32_t longitude = gnss_get_longitude();      // degrees * 10^-7
        int32_t altitude = gnss_get_altitude_msl();    // millimeters
        int32_t speed = gnss_get_ground_speed();       // mm/s
        int32_t heading = gnss_get_heading();          // degrees * 10^-5
        uint8_t satellites = gnss_get_siv();           // number of satellites
        
        // Use data as needed
        float lat_f = latitude / 10000000.0f;
        float lon_f = longitude / 10000000.0f;
        float alt_m = altitude / 1000.0f;
        float spd_ms = speed / 1000.0f;
        
        // Send telemetry, log to SD card, etc.
    } else {
        // No fix yet
        uint8_t siv = gnss_get_siv();
        printf("Waiting for GNSS fix... SIV: %d\n", siv);
    }
*/

// ============================================================================
// COMPLETE EXAMPLE - MINIMAL INTEGRATION
// ============================================================================

/*
MINIMAL CODE TO ADD TO user_main.cpp:

// At top with includes:
#include "ublox_gnss_example.h"

// Global state:
static bool gnss_ready = false;
static uint32_t gnss_last_update = 0;

// Initialization (call once):
void gnss_setup(void) {
    gnss_ready = gnss_init(&huart3, &huart3_IT_ready);
    printf("GNSS: %s\n", gnss_ready ? "OK" : "FAILED");
}

// Periodic update (call every 10-100ms):
void gnss_update_task(void) {
    if (!gnss_ready) return;
    
    uint32_t now = millis();
    if ((now - gnss_last_update) >= 100) {
        gnss_update();
        gnss_last_update = now;
    }
}

// In your existing main/loop, add:
gnss_setup();      // Once at startup
gnss_update_task(); // Frequently in loop
*/

#endif
