/**
 * @file ublox_gnss_example.cpp
 * @brief Example implementation and API for SparkFun u-blox GNSS with STM32 HAL
 */

#include <stdio.h>
#include "mySerial.h"
#include "ublox_gnss_wrapper.h"

#include "stm32_arduino_compatibility.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Global Variables
// ============================================================================

extern mySerial gnssSerialWrapper;  // UART3 mySerial wrapper (from user_main.cpp)
extern STM32Stream* gnssSerial;     // UART3 STM32Stream wrapper (from user_main.cpp)
extern UbloxGNSSWrapper *pGNSS;
static uint32_t lastUpdateTime = 0;
static const uint32_t UPDATE_INTERVAL_MS = 100;

// ============================================================================
// Public API Implementation
// ============================================================================

bool gnss_init(UART_HandleTypeDef *huart3) {
    if (!huart3) {
        return false;
    }
    
    // Initialize the mySerial wrapper (only needs to be done once)
    // Check if the UART handle matches to avoid re-initialization
    static bool initialized = false;
    if (!initialized) {
        gnssSerialWrapper.init(huart3, huart_TX_ready, huart_RX_ready, 512, 8);
        initialized = true;
    }
    
    // Create STM32Stream wrapper if not already created
    if (!gnssSerial) {
        gnssSerial = new STM32Stream(&gnssSerialWrapper);
    }
    
    // Create the UbloxGNSSWrapper with the STM32Stream
    pGNSS = new UbloxGNSSWrapper(*gnssSerial);
    if (!pGNSS) {
        return false;
    }
    
    if (!pGNSS->begin(100)) {
        printf("GNSS init failed\n\r");
        return false;
    }
    
    printf("GNSS initialized successfully\n\r");
    lastUpdateTime = millis();
    return true;
}

void gnss_update(void) {
    uint32_t now = millis();
    
    if ((now - lastUpdateTime) >= UPDATE_INTERVAL_MS) {
        if (pGNSS) {
            pGNSS->update();
            lastUpdateTime = now;
        }
    }
    
  //  gnssSerial.updateSerial();
}

int32_t gnss_get_latitude(void) {
    return pGNSS ? pGNSS->getLatitude() : 0;
}

int32_t gnss_get_longitude(void) {
    return pGNSS ? pGNSS->getLongitude() : 0;
}

int32_t gnss_get_altitude_msl(void) {
    return pGNSS ? pGNSS->getAltitudeMSL() : 0;
}

int32_t gnss_get_ground_speed(void) {
    return pGNSS ? pGNSS->getGroundSpeed() : 0;
}

int32_t gnss_get_heading(void) {
    return pGNSS ? pGNSS->getHeading() : 0;
}

uint8_t gnss_get_siv(void) {
    return pGNSS ? pGNSS->getSIV() : 0;
}

bool gnss_has_valid_fix(void) {
    return pGNSS ? pGNSS->hasValidFix() : false;
}

#ifdef __cplusplus
}  // extern "C"
#endif

SFE_UBLOX_GNSS& gnss_get_raw(void) {
    static SFE_UBLOX_GNSS dummy;
    return pGNSS ? pGNSS->getGNSS() : dummy;
}
