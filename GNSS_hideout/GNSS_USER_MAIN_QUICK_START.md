# GNSS Integration to user_main.h / user_main.cpp - Quick Reference

## Minimal Steps (Copy-Paste Ready)

### Step 1: Add to user_main.h

In the includes section, add:
```cpp
#include "ublox_gnss_example.h"
```

In the extern "C" section, add:
```cpp
void gnss_module_init(void);
void gnss_module_update(uint32_t millis_now);
```

### Step 2: Add to user_main.cpp - Global Variables

```cpp
// GNSS module state variables
static bool gnss_initialized = false;
static uint32_t gnss_last_update_time = 0;
static const uint32_t GNSS_UPDATE_INTERVAL_MS = 100;  // 10Hz
```

### Step 3: Add to user_main.cpp - Initialization Function

```cpp
void gnss_module_init(void) {
    printf("Initializing GNSS Module...\n");
    gnss_initialized = gnss_init(&huart3, &huart3_IT_ready);
    if (gnss_initialized) {
        printf("GNSS: OK\n");
        gnss_last_update_time = millis();
    } else {
        printf("GNSS: FAILED - Check UART3 configuration\n");
    }
}
```

### Step 4: Add to user_main.cpp - Update Function

```cpp
void gnss_module_update(uint32_t millis_now) {
    if (!gnss_initialized) return;
    
    if ((millis_now - gnss_last_update_time) >= GNSS_UPDATE_INTERVAL_MS) {
        gnss_update();  // Non-blocking
        gnss_last_update_time = millis_now;
    }
}
```

### Step 5: Call in Your Existing Code

In your `user_init()` or equivalent setup function:
```cpp
gnss_module_init();  // Once at startup (blocking is OK here)
```

In your main loop:
```cpp
void user_loop(uint32_t millis_now) {
    // ... existing code ...
    gnss_module_update(millis_now);  // Call frequently (non-blocking)
    // ... rest of code ...
}
```

### Step 6: Use Data

Anywhere in your code where you need position:
```cpp
if (gnss_has_valid_fix()) {
    int32_t lat = gnss_get_latitude();      // degrees * 10^-7
    int32_t lon = gnss_get_longitude();     // degrees * 10^-7
    int32_t alt = gnss_get_altitude_msl();  // millimeters
    uint8_t siv = gnss_get_siv();           // satellites
    
    // Convert to human-readable format
    float lat_deg = lat / 10000000.0f;
    float lon_deg = lon / 10000000.0f;
    float alt_m = alt / 1000.0f;
    
    printf("Position: %.7f, %.7f, Alt: %.1fm, SIV: %d\n",
           lat_deg, lon_deg, alt_m, siv);
}
```

## Data Types and Conversions

| Function | Returns | Units | Convert To |
|----------|---------|-------|-----------|
| `gnss_get_latitude()` | `int32_t` | degrees × 10⁻⁷ | ÷ 10,000,000 = decimal degrees |
| `gnss_get_longitude()` | `int32_t` | degrees × 10⁻⁷ | ÷ 10,000,000 = decimal degrees |
| `gnss_get_altitude_msl()` | `int32_t` | millimeters | ÷ 1,000 = meters |
| `gnss_get_ground_speed()` | `int32_t` | mm/s | ÷ 1,000 = m/s |
| `gnss_get_heading()` | `int32_t` | degrees × 10⁻⁵ | ÷ 100,000 = decimal degrees |
| `gnss_get_siv()` | `uint8_t` | count | 0 = no fix, 4+ = valid 3D |
| `gnss_has_valid_fix()` | `bool` | - | true if SIV > 0 |

## Important Notes

✅ **Non-blocking**: After `gnss_module_init()`, all calls are non-blocking
✅ **Fast getters**: `gnss_get_*()` functions return in < 1 microsecond
✅ **Frequent updates**: Call `gnss_module_update()` at least every 100ms
✅ **No main.c changes**: Everything stays in user_main.cpp
✅ **UART3 required**: Uses UART3 for GNSS module communication

⚠️ **Init is blocking**: `gnss_module_init()` blocks 1-2 seconds (OK during setup)
⚠️ **Cold start time**: First fix takes 30-60 seconds without prior ephemeris data
⚠️ **Clear sky**: GNSS needs antenna with view of sky

## Integration Summary

| Step | Function | Time | Blocking |
|------|----------|------|----------|
| 1 | `gnss_module_init()` | ~1-2s | Yes (setup only) |
| 2 | `gnss_module_update()` | ~5ms | No (call frequently) |
| 3 | `gnss_get_*()` | <1μs | No (instant) |

## Troubleshooting

**GNSS fails to initialize:**
- Check UART3 enabled in STM32CubeMX
- Check baud rate (38400 or 115200)
- Check antenna connection
- Use logic analyzer to verify UART3 RX data

**No fix after 1+ minute:**
- Move antenna outside or near window
- Antenna needs clear view of sky
- Cold start takes 30+ seconds
- Check SIV value increasing (0 → 4+)

**Data not updating:**
- Ensure `gnss_module_update()` called every 10-100ms
- Check output shows increasing SIV value
- Monitor with logic analyzer on UART3 RX line

## File References

- **See**: `GNSS_USER_MAIN_EXAMPLE.cpp` - Complete copy-paste ready code
- **See**: `USER_MAIN_GNSS_INTEGRATION.md` - Detailed integration guide
- **See**: `GNSS_INTEGRATION_GUIDE.md` - Full technical documentation
- **See**: `ARCHITECTURE_DIAGRAM.md` - System architecture details

## Example: Complete 50-Line Integration

```cpp
// user_main.h changes:
#include "ublox_gnss_example.h"
void gnss_module_init(void);
void gnss_module_update(uint32_t ms);

// user_main.cpp changes:
static bool gnss_ok = false;
static uint32_t gnss_tm = 0;

void gnss_module_init(void) {
    gnss_ok = gnss_init(&huart3, &huart3_IT_ready);
    gnss_tm = millis();
}

void gnss_module_update(uint32_t ms) {
    if (!gnss_ok) return;
    if ((ms - gnss_tm) >= 100) {
        gnss_update();
        gnss_tm = ms;
    }
}

// In user_init():
gnss_module_init();

// In user_loop(ms):
gnss_module_update(ms);
if (gnss_has_valid_fix()) {
    printf("Lat: %ld\n", gnss_get_latitude());
}
```

That's it! You now have working GNSS integration without touching main.c.
