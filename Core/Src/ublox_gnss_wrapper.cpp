/**
 * @file ublox_gnss_wrapper.cpp
 * @brief Implementation of non-blocking wrapper for SFE_UBLOX_GNSS
 */

#include "ublox_gnss_wrapper.h"
#include "stm32_arduino_compatibility.h"

UbloxGNSSWrapper::UbloxGNSSWrapper(Stream &serialPort) 
    : serialPort(&serialPort), cachedLatitude(0), cachedLongitude(0), cachedAltitude(0),
      cachedSpeed(0), cachedHeading(0), cachedSIV(0), 
      cachedValidFix(false), lastUpdateTime(0) {
    // Store the serial port reference for later use
}

bool UbloxGNSSWrapper::begin(uint16_t maxWait) {
    // This is a blocking call, safe to use during initialization
    // Use the stored serial port reference
    if (!serialPort) {
        return false; // Serial port not initialized
    }
    
    // Begin communication with GNSS module
    bool success = myGNSS.begin(*serialPort, maxWait, true);
    
    if (success) {
        // Configure for non-blocking operation
        // Enable automatic PVT reporting at 1Hz
        myGNSS.setAutoPVT(true, maxWait);
        
        // Set measurement rate to 500ms (2Hz)
        myGNSS.setMeasurementRate(500, maxWait);
        
        // Configure UART1 output to UBX only (reduces baudrate)
        myGNSS.setUART1Output(COM_TYPE_UBX, maxWait);
    }
    
    return success;
}

void UbloxGNSSWrapper::update(void) {
    // Non-blocking update - just check for new data
    // The checkUblox method is non-blocking when autoPVT is enabled
    myGNSS.checkUblox();
    
    // Cache the current values (these are updated automatically when autoPVT is enabled)
    cachedLatitude = myGNSS.getLatitude(0);   // maxWait = 0 means non-blocking
    cachedLongitude = myGNSS.getLongitude(0);
    cachedAltitude = myGNSS.getAltitudeMSL(0);
    cachedSpeed = myGNSS.getGroundSpeed(0);
    cachedHeading = myGNSS.getHeading(0);
    cachedSIV = myGNSS.getSIV(0);
    
    // Determine if we have a valid fix
    cachedValidFix = (cachedSIV > 0);
    
    lastUpdateTime = millis();
}

void UbloxGNSSWrapper::setSerialRate(uint32_t baudrate, uint16_t maxWait) {
    myGNSS.setSerialRate(baudrate, COM_PORT_UART1, maxWait);
}

bool UbloxGNSSWrapper::setAutoPVT(bool enabled, uint16_t maxWait) {
    return myGNSS.setAutoPVT(enabled, maxWait);
}

bool UbloxGNSSWrapper::setMeasurementRate(uint16_t rate, uint16_t maxWait) {
    return myGNSS.setMeasurementRate(rate, maxWait);
}

bool UbloxGNSSWrapper::setUART1Output(uint8_t comSettings, uint16_t maxWait) {
    return myGNSS.setUART1Output(comSettings, maxWait);
}

bool UbloxGNSSWrapper::getPVT(uint16_t maxWait) {
    // If autoPVT is enabled, this returns without blocking
    // If autoPVT is disabled, this polls and waits up to maxWait
    return myGNSS.getPVT(maxWait);
}

// ============================================================================
// Non-blocking Getter Methods
// ============================================================================

int32_t UbloxGNSSWrapper::getLatitude(void) {
    return cachedLatitude;
}

int32_t UbloxGNSSWrapper::getLongitude(void) {
    return cachedLongitude;
}

int32_t UbloxGNSSWrapper::getAltitudeMSL(void) {
    return cachedAltitude;
}

int32_t UbloxGNSSWrapper::getGroundSpeed(void) {
    return cachedSpeed;
}

int32_t UbloxGNSSWrapper::getHeading(void) {
    return cachedHeading;
}

uint8_t UbloxGNSSWrapper::getSIV(void) {
    return cachedSIV;
}

bool UbloxGNSSWrapper::hasValidFix(void) {
    return cachedValidFix;
}

SFE_UBLOX_GNSS& UbloxGNSSWrapper::getGNSS(void) {
    return myGNSS;
}
