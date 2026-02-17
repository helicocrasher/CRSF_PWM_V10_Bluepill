# GNSS Integration Checklist - user_main.h/cpp Only

## Pre-Integration

- [ ] All 6 GNSS glue files added to project:
  - [ ] Core/Inc/stm32_arduino_compatibility.h
  - [ ] Core/Src/stm32_arduino_compatibility.cpp
  - [ ] Core/Inc/ublox_gnss_wrapper.h
  - [ ] Core/Src/ublox_gnss_wrapper.cpp
  - [ ] Core/Inc/ublox_gnss_example.h
  - [ ] Core/Src/ublox_gnss_example.cpp
  
- [ ] SparkFun library header modified:
  - [ ] SparkFun_u-blox_GNSS_Arduino_Library.h: Arduino.h → stm32_arduino_compatibility.h

- [ ] Project settings:
  - [ ] Include path includes Core/Inc
  - [ ] C++ files are compiled (not just C)
  - [ ] No conflicting Arduino.h includes

## user_main.h Integration

- [ ] Add include:
  ```cpp
  #include "ublox_gnss_example.h"
  ```

- [ ] Add function declarations in extern "C" block:
  ```cpp
  void gnss_module_init(void);
  void gnss_module_update(uint32_t millis_now);
  ```

## user_main.cpp Integration

### Global Variables
- [ ] Add at module level (after other statics):
  ```cpp
  static bool gnss_initialized = false;
  static uint32_t gnss_last_update_time = 0;
  static const uint32_t GNSS_UPDATE_INTERVAL_MS = 100;
  ```

### Initialization Function
- [ ] Add `gnss_module_init()` function:
  ```cpp
  void gnss_module_init(void) {
      gnss_initialized = gnss_init(&huart3, &huart3_IT_ready);
      gnss_last_update_time = millis();
  }
  ```

### Update Function
- [ ] Add `gnss_module_update()` function:
  ```cpp
  void gnss_module_update(uint32_t millis_now) {
      if (!gnss_initialized) return;
      if ((millis_now - gnss_last_update_time) >= GNSS_UPDATE_INTERVAL_MS) {
          gnss_update();
          gnss_last_update_time = millis_now;
      }
  }
  ```

## Integration into Existing Code

### During Setup
- [ ] Call `gnss_module_init()` once:
  ```cpp
  // In user_init() or equivalent startup function
  gnss_module_init();  // Blocking, ~1-2 seconds
  ```

### During Main Loop
- [ ] Add `gnss_module_update()` call:
  ```cpp
  // In user_loop() or periodic task
  gnss_module_update(millis_now);  // Non-blocking
  ```

## Testing

### Compilation
- [ ] No compilation errors
- [ ] No undefined reference errors
- [ ] No conflicting symbol errors

### Runtime
- [ ] GNSS initialization message appears
- [ ] UART3 is responsive (check with logic analyzer if needed)
- [ ] After 30-60 seconds, SIV increases (satellites acquired)
- [ ] Position data updates periodically

### Data Access (Optional)
- [ ] Add test code to read GNSS data:
  ```cpp
  if (gnss_has_valid_fix()) {
      printf("Lat: %ld\n", gnss_get_latitude());
  }
  ```

## Integration Checklist Summary

### Files to Add (6 files)
- [ ] stm32_arduino_compatibility.h/cpp
- [ ] ublox_gnss_wrapper.h/cpp
- [ ] ublox_gnss_example.h/cpp

### Files to Modify (2 files)
- [ ] user_main.h - Add include + declarations
- [ ] user_main.cpp - Add functions + calls

### Files to NOT Modify
- [x] main.c - No changes needed
- [x] SparkFun library .cpp files - No changes

## Quick Validation

After integration, your code should have:

1. **In user_main.h:**
   - `#include "ublox_gnss_example.h"`
   - `void gnss_module_init(void);`
   - `void gnss_module_update(uint32_t);`

2. **In user_main.cpp:**
   - Global: `gnss_initialized`, `gnss_last_update_time`
   - Function: `gnss_module_init()`
   - Function: `gnss_module_update()`
   - Call: `gnss_module_init()` in setup
   - Call: `gnss_module_update(ms)` in loop

3. **Data access anywhere:**
   - `gnss_has_valid_fix()`
   - `gnss_get_latitude()`
   - `gnss_get_longitude()`
   - `gnss_get_siv()`
   - etc.

## Build and Test

```bash
# Build project
[ Build output should be clean ]

# Flash to board
[ Upload firmware ]

# Monitor serial output
[ Should see "Initializing GNSS..." ]
[ Should see position updates after ~30-60 seconds ]
```

## Troubleshooting Checklist

| Issue | Check |
|-------|-------|
| Compilation error | All 6 files added? Include path correct? |
| Undefined reference | .cpp files compiled? C++ enabled in project? |
| Init fails | UART3 enabled in CubeMX? Baud rate set? |
| No fix | Antenna connected? Outside location? Waited 60s? |
| No updates | `gnss_module_update()` called frequently? |

## Common Issues and Solutions

**Issue**: `undefined reference to gnss_init`
- **Fix**: Ensure `ublox_gnss_example.cpp` is in build

**Issue**: `stm32_arduino_compatibility.h: No such file`
- **Fix**: Check include path includes Core/Inc

**Issue**: Conflicting Arduino.h
- **Fix**: Verify SparkFun header modification

**Issue**: GNSS doesn't initialize
- **Fix**: Check UART3 in STM32CubeMX

## Next Steps

After successful integration:

1. ✅ Read position data with `gnss_get_latitude()`, etc.
2. ✅ Send to telemetry system if needed
3. ✅ Log to SD card if available
4. ✅ Integrate with CRSF/RC transmitter
5. ✅ Fine-tune update rates as needed

## Reference Files

- `GNSS_USER_MAIN_QUICK_START.md` - Copy-paste ready code
- `GNSS_USER_MAIN_EXAMPLE.cpp` - Complete example
- `USER_MAIN_GNSS_INTEGRATION.md` - Detailed guide
- `GNSS_INTEGRATION_GUIDE.md` - Full technical docs

---

**Status**: Ready for integration
**Effort**: ~15 minutes to add all integration code
**Complexity**: Low - mostly copy-paste
**Blocking**: Only init (1-2s), everything else non-blocking
