# SparkFun u-blox GNSS Integration Guide for STM32 HAL

## Overview

This integration provides a non-blocking interface to the SparkFun u-blox GNSS Arduino Library for STM32 HAL projects without the Arduino framework. The solution uses:

- **stm32_arduino_compatibility**: Provides Arduino-like abstractions (Stream, Wire, SPI stubs)
- **ublox_gnss_wrapper**: Non-blocking wrapper around SFE_UBLOX_GNSS
- **ublox_gnss_example**: High-level API for easy integration
- **myHalfSerial_X**: Existing UART abstraction from your project (RX-based)

## Architecture

```
┌─────────────────────────────────────────┐
│      Main Application Code              │
│   (uses gnss_get_*() functions)        │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    ublox_gnss_example                   │
│   (High-level C API)                    │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    UbloxGNSSWrapper (C++)               │
│   (Non-blocking wrapper)                │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    SFE_UBLOX_GNSS (SparkFun)           │
│   (Original library)                    │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    STM32Serial (Stream wrapper)        │
│   (Arduino Stream compatibility)        │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    myHalfSerial_X                       │
│   (STM32 UART abstraction layer)       │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    STM32 HAL UART3                     │
│   (Low-level hardware)                  │
└─────────────────────────────────────────┘
```

## Files Created/Modified

### New Files
1. **Core/Inc/stm32_arduino_compatibility.h** - Arduino compatibility layer header
2. **Core/Src/stm32_arduino_compatibility.cpp** - Arduino compatibility implementation
3. **Core/Inc/ublox_gnss_wrapper.h** - Non-blocking GNSS wrapper header
4. **Core/Src/ublox_gnss_wrapper.cpp** - Non-blocking GNSS wrapper implementation
5. **Core/Inc/ublox_gnss_example.h** - High-level API header
6. **Core/Src/ublox_gnss_example.cpp** - High-level API implementation

### Modified Files
1. **SparkFun_u-blox_GNSS_Arduino_Library/src/SparkFun_u-blox_GNSS_Arduino_Library.h**
   - Changed Arduino.h includes to stm32_arduino_compatibility.h
   - Wire.h and SPI.h now provided by compatibility layer

## Integration Steps

### Step 1: STM32CubeMX Configuration

1. Open your STM32CubeMX project
2. Configure UART3:
   - Enable UART3 peripheral
   - Set baud rate to match your GNSS module (typically 38400 or 115200)
   - **Enable RX interrupt** (UART3_RX global interrupt)
   - Disable TX (GNSS is RX-only for this application)
3. Generate code

### Step 2: Update UART3 Interrupt Handler

In `stm32f1xx_it.c`, locate the `UART3_IRQHandler` and ensure it calls the proper HAL handler:

```c
void UART3_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart3);
}
```

If not present, add this handler.

### Step 3: Add Files to Your Project

Copy the following files:
- Core/Inc/stm32_arduino_compatibility.h
- Core/Src/stm32_arduino_compatibility.cpp
- Core/Inc/ublox_gnss_wrapper.h
- Core/Src/ublox_gnss_wrapper.cpp
- Core/Inc/ublox_gnss_example.h
- Core/Src/ublox_gnss_example.cpp

### Step 4: Update Your Main Function

In `Core/Src/main.c` or your equivalent main file:

```c
#include "ublox_gnss_example.h"

int main(void) {
    // HAL initialization (from STM32CubeMX)
    HAL_Init();
    SystemClock_Config();
    
    // Initialize peripherals
    MX_GPIO_Init();
    MX_UART1_Init();     // Debug/logging
    MX_UART3_Init();     // GNSS module
    
    // Initialize GNSS module
    if (!gnss_init(&huart3, &huart3_IT_ready)) {
        printf("GNSS initialization failed\n");
        // Handle error - maybe use fallback mode
    }
    
    // Main loop
    while (1) {
        // Update GNSS data (non-blocking, should be called frequently)
        // Recommended: 10-100Hz (every 10-100ms)
        gnss_update();
        
        // Your other application code here...
        
        // Example: Print position every second
        static uint32_t lastPrintTime = 0;
        if (millis() - lastPrintTime > 1000) {
            if (gnss_has_valid_fix()) {
                int32_t lat = gnss_get_latitude();
                int32_t lon = gnss_get_longitude();
                uint8_t siv = gnss_get_siv();
                int32_t alt = gnss_get_altitude_msl();
                
                printf("Lat: %ld.%07ld, Lon: %ld.%07ld, Alt: %ldm, SIV: %d\n",
                       lat / 10000000, lat % 10000000 / 10,
                       lon / 10000000, lon % 10000000 / 10,
                       alt / 1000, siv);
            } else {
                printf("Waiting for GNSS fix...\n");
            }
            lastPrintTime = millis();
        }
    }
    
    return 0;
}
```

### Step 5: Update Include Paths (if needed)

If your include paths don't already include `Core/Inc`, add it:
- In STM32CubeMX: Project Manager → Settings → C/C++ → Include Paths
- Or in your IDE's project settings

## API Reference

### Initialization
```c
// Must be called once during setup (blocking, ~1-2 seconds)
bool gnss_init(UART_HandleTypeDef *huart3, bool *huart_IT_ready);
```

### Main Loop Update
```c
// Call frequently (recommended: 10-100 Hz, every 10-100 ms)
// Non-blocking - returns immediately
void gnss_update(void);
```

### Data Access (All Non-Blocking)
```c
// Returns latitude in degrees * 10^-7
// Example: 40.123456° = 401234560
int32_t gnss_get_latitude(void);

// Returns longitude in degrees * 10^-7
int32_t gnss_get_longitude(void);

// Returns altitude above mean sea level in millimeters
int32_t gnss_get_altitude_msl(void);

// Returns ground speed in mm/s
int32_t gnss_get_ground_speed(void);

// Returns heading in degrees * 10^-5
// Example: 120.345° = 12034500
int32_t gnss_get_heading(void);

// Returns number of satellites in use (0 = no fix)
uint8_t gnss_get_siv(void);

// Returns true if we have valid fix (SIV > 0)
bool gnss_has_valid_fix(void);
```

## Non-Blocking Behavior

The implementation ensures all calls outside initialization are non-blocking:

1. **gnss_update()**: Polls UART for new data, caches values. Returns immediately.
2. **gnss_get_*()**: Returns cached values. Returns immediately.
3. **gnss_init()**: Blocking only, ~1-2 seconds max. Call once during setup.

## Speed Conversion

Divide by 1000.0 to convert mm/s to m/s:
```c
int32_t speed_mm_s = gnss_get_ground_speed();
float speed_m_s = speed_mm_s / 1000.0;
```

## Heading Conversion

Divide by 100000.0 to convert to degrees:
```c
int32_t heading_raw = gnss_get_heading();
float heading_deg = heading_raw / 100000.0;
```

## Coordinates Conversion

Divide by 10000000.0 to convert to decimal degrees:
```c
int32_t lat_raw = gnss_get_latitude();
float lat_deg = lat_raw / 10000000.0;
```

## Troubleshooting

### GNSS Module Not Communicating
1. Check UART3 configuration in STM32CubeMX
2. Verify baud rate matches GNSS module (default usually 38400)
3. Check that UART3_RX is mapped to correct pin
4. Verify UART3 interrupt is enabled and handler present

### GNSS Not Getting Fix
1. Wait 30-60 seconds for cold start
2. Ensure antenna is outside with clear sky view
3. Check that autoPVT is enabled (done in gnss_init by default)
4. Monitor gnss_get_siv() - should increase from 0 when satellites are visible

### Data Not Updating
1. Ensure gnss_update() is called frequently (every 10-100ms)
2. Check that myHalfSerial_X.updateSerial() is being called
3. Verify UART RX interrupt is firing and calling HAL_UART_IRQHandler

## Advanced Usage

If you need direct access to the underlying SFE_UBLOX_GNSS object:

```cpp
#include "ublox_gnss_example.h"

// In C++ code:
SFE_UBLOX_GNSS& gnss = gnss_get_raw();
// Now you can use all SparkFun library methods directly
```

## Performance Notes

- **Latency**: Data latency is ~1 GNSS cycle (~1 second) when update() is called every 100ms
- **Memory**: Total memory usage ~2-3 KB for GNSS module internal structures
- **CPU**: update() call is fast (~1-10ms typical), mostly spent polling UART
- **UART Buffer**: 256-byte FIFO should handle ~100ms of data at 115200 baud

## Limitations

1. **I2C Not Supported**: Only serial/UART communication is supported
2. **SPI Not Supported**: Only serial/UART communication is supported
3. **No Hardware Flow Control**: Assumes GNSS module doesn't require RTS/CTS
4. **RX Only**: Current implementation assumes GNSS is RX-only (this is typical for u-blox modules)

## References

- [SparkFun u-blox GNSS Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library)
- [u-blox u-center Guide](https://www.u-blox.com/en/product/u-center)
- [STM32 HAL Documentation](https://www.st.com/resource/en/user_manual/dm00154567-stm32cubemx-stm32mx-and-stm32l0-series-stm32cube-initialization-code-generator-stmicroelectronics.pdf)

## Support

For issues:
1. Check that all files are included in your project
2. Verify include paths are correct
3. Check compiler warnings/errors in build output
4. Enable debug logging by uncommenting `printf()` statements
5. Monitor UART data using a logic analyzer or serial monitor

---
**Last Updated**: February 2026
**Compatibility**: STM32F1 HAL, SparkFun u-blox GNSS Library v3.0+
