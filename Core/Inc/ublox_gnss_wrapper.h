/**
 * @file ublox_gnss_wrapper.h
 * @brief Non-blocking wrapper for SFE_UBLOX_GNSS for STM32 HAL UART operation
 * 
 * This wrapper class provides a non-blocking interface to the SparkFun u-blox GNSS
 * library. All getter methods return immediately with cached values. The GNSS module
 * must be polled periodically using the update() method.
 * 
 * Usage:
 * 1. Initialize myHalfSerial_X for UART3
 * 2. Create UbloxGNSSWrapper instance
 * 3. Call begin() once during setup (blocking, ~1 second max)
 * 4. Call update() periodically from main loop (non-blocking)
 * 5. Call getter methods to access cached data (non-blocking)
 */

#ifndef UBLOX_GNSS_WRAPPER_H
#define UBLOX_GNSS_WRAPPER_H

#include "SparkFun_u-blox_GNSS_Arduino_Library.h"

class UbloxGNSSWrapper {
public:
    /**
     * @brief Constructor
     * @param serialPort: Pointer to STM32Serial instance connected to UART3
     */
    UbloxGNSSWrapper(Stream &serialPort);
    
    /**
     * @brief Initialize GNSS module (BLOCKING - safe to call during setup only)
     * @param maxWait: Maximum time to wait for initialization in ms
     * @return true if initialization successful, false otherwise
     * 
     * This call blocks for up to maxWait milliseconds. It should only be used during
     * system initialization, not in the main loop.
     */
    bool begin(uint16_t maxWait = 1000);
    
    /**
     * @brief Update GNSS data (NON-BLOCKING)
     * 
     * Call this periodically (e.g., every 100ms) from main loop. It polls the module
     * for new data without blocking. Returns immediately.
     */
    void update(void);
    
    // ========================================================================
    // Configuration Methods (can block during setup)
    // ========================================================================
    
    /**
     * @brief Set UART baud rate
     * @param baudrate: Target baud rate (e.g., 115200)
     * @param maxWait: Maximum time to wait in ms
     */
    void setSerialRate(uint32_t baudrate, uint16_t maxWait = 1000);
    
    /**
     * @brief Enable/disable automatic PVT reports
     * @param enabled: true to enable, false to disable
     * @param maxWait: Maximum time to wait in ms
     * @return true if successful
     */
    bool setAutoPVT(bool enabled, uint16_t maxWait = 1000);
    
    /**
     * @brief Set measurement rate
     * @param rate: Time between measurements in milliseconds
     * @param maxWait: Maximum time to wait in ms
     * @return true if successful
     */
    bool setMeasurementRate(uint16_t rate, uint16_t maxWait = 1000);
    
    /**
     * @brief Configure UART1 output
     * @param comSettings: COM_TYPE_UBX, COM_TYPE_NMEA, etc.
     * @param maxWait: Maximum time to wait in ms
     * @return true if successful
     */
    bool setUART1Output(uint8_t comSettings, uint16_t maxWait = 1000);
    
    /**
     * @brief Poll for latest PVT data (explicit poll if autoPVT disabled)
     * @param maxWait: Max wait time in ms (when autoPVT disabled)
     * @return true if new data available
     */
    bool getPVT(uint16_t maxWait = 100);
    
    // ========================================================================
    // Getter Methods (NON-BLOCKING - return cached values)
    // ========================================================================
    
    /**
     * @brief Get latitude (cached from last successful update)
     * @return Latitude in degrees * 10^-7 (e.g., 40.123456° = 401234560)
     */
    int32_t getLatitude(void);
    
    /**
     * @brief Get longitude (cached from last successful update)
     * @return Longitude in degrees * 10^-7
     */
    int32_t getLongitude(void);
    
    /**
     * @brief Get altitude above mean sea level (cached)
     * @return Altitude in millimeters
     */
    int32_t getAltitudeMSL(void);
    
    /**
     * @brief Get ground speed (cached)
     * @return Speed in mm/s
     */
    int32_t getGroundSpeed(void);
    
    /**
     * @brief Get heading (cached)
     * @return Heading in degrees * 10^-5
     */
    int32_t getHeading(void);
    
    /**
     * @brief Get number of satellites in use (cached)
     * @return Number of satellites (0-99)
     */
    uint8_t getSIV(void);
    
    /**
     * @brief Check if we have a valid fix
     * @return true if fix is valid (SIV > 0)
     */
    bool hasValidFix(void);
    
    /**
     * @brief Get the underlying SFE_UBLOX_GNSS object for advanced use
     * @return Reference to the GNSS object
     */
    SFE_UBLOX_GNSS& getGNSS(void);

private:
    SFE_UBLOX_GNSS myGNSS;
    
    // Cached data (updated by update())
    int32_t cachedLatitude;
    int32_t cachedLongitude;
    int32_t cachedAltitude;
    int32_t cachedSpeed;
    int32_t cachedHeading;
    uint8_t cachedSIV;
    bool cachedValidFix;
    
    uint32_t lastUpdateTime;
};

#endif // UBLOX_GNSS_WRAPPER_H
