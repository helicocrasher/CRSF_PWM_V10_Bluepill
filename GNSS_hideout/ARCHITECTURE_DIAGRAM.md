# SparkFun u-blox GNSS Integration - Architecture & Data Flow

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    YOUR APPLICATION CODE                        │
│  - Main loop                                                    │
│  - Background tasks                                             │
│  - Interrupt handlers                                           │
└──────────────────┬──────────────────────────────────────────────┘
                   │ gnss_init(), gnss_update(), gnss_get_*()
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│              HIGH-LEVEL C API (ublox_gnss_example)              │
│  - gnss_init(huart, ready_flag)      [BLOCKING, 1-2s]          │
│  - gnss_update()                     [NON-BLOCKING]            │
│  - gnss_get_latitude(), etc.         [NON-BLOCKING]            │
│  - Global: gnssSerial, pGNSS                                   │
└──────────────────┬──────────────────────────────────────────────┘
                   │ Data cached internally
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│          NON-BLOCKING WRAPPER (UbloxGNSSWrapper)               │
│  - Data cache: lat, lon, alt, speed, heading, siv             │
│  - GNSS object: SFE_UBLOX_GNSS myGNSS                         │
│  - Methods: update(), getLatitude(), etc.                      │
│  - Uses: checkUblox() for polling                              │
└──────────────────┬──────────────────────────────────────────────┘
                   │ SFE_UBLOX_GNSS interface
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│     SPARKFUN GNSS LIBRARY (SFE_UBLOX_GNSS)                     │
│  - Packet parsing                                              │
│  - Command/response handling                                   │
│  - UBX protocol implementation                                 │
│  - Uses: Stream interface                                      │
└──────────────────┬──────────────────────────────────────────────┘
                   │ Stream read/write
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│   ARDUINO COMPATIBILITY LAYER (stm32_arduino_compatibility)    │
│  - STM32Serial: Stream implementation                          │
│  - Uses: myHalfSerial_X for UART                              │
│  - Timing: millis(), micros(), delay()                        │
│  - Stubs: Wire (I2C), SPI                                     │
└──────────────────┬──────────────────────────────────────────────┘
                   │ UART read/write
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│              UART INTERFACE (myHalfSerial_X)                    │
│  - FIFO buffering                                              │
│  - Interrupt-driven reception                                  │
│  - Non-blocking read/write                                     │
│  - Uses: STM32 UART3 HAL                                      │
└──────────────────┬──────────────────────────────────────────────┘
                   │ Hardware UART
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│               STM32 HAL UART MODULE                             │
│  - UART3 peripheral                                            │
│  - Interrupt handling (RX)                                     │
│  - Baud rate: 38400 or 115200                                 │
└──────────────────┬──────────────────────────────────────────────┘
                   │ RS232 / RS422 connection
                   │
┌──────────────────▼──────────────────────────────────────────────┐
│              GNSS MODULE (u-blox NEU-M9N, etc.)                │
│  - Satellite reception                                         │
│  - Position calculation                                        │
│  - UBX packet transmission on UART                             │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow: gnss_update() Call

```
Main Loop
   │
   ├─→ gnss_update()
   │   │
   │   ├─→ Check interval (every 100ms by default)
   │   │
   │   ├─→ myGNSS.checkUblox()
   │   │   │
   │   │   ├─→ checkUbloxSerial()
   │   │   │   │
   │   │   │   ├─→ Stream::available()
   │   │   │   │   ↓
   │   │   │   │   myHalfSerial_X::available()
   │   │   │   │   ↓
   │   │   │   │   Returns buffered byte count
   │   │   │   │
   │   │   │   └─→ Stream::read()
   │   │   │       ↓
   │   │   │       myHalfSerial_X::read()
   │   │   │       ↓
   │   │   │       Returns data from FIFO
   │   │   │
   │   │   └─→ Process UBX packets
   │   │
   │   └─→ Cache all GPS values
   │       ├─ cachedLatitude = myGNSS.getLatitude(0)
   │       ├─ cachedLongitude = myGNSS.getLongitude(0)
   │       ├─ cachedAltitude = myGNSS.getAltitudeMSL(0)
   │       ├─ cachedSpeed = myGNSS.getGroundSpeed(0)
   │       ├─ cachedHeading = myGNSS.getHeading(0)
   │       └─ cachedSIV = myGNSS.getSIV(0)
   │
   ↓ Returns immediately (non-blocking)
```

## UART3 Receive Interrupt Flow

```
GNSS Module sends UBX packet
   │
   ↓ UART3 interrupt on each byte
   │
STM32 HAL: UART3_IRQHandler()
   │
   ├─→ HAL_UART_IRQHandler(&huart3)
   │
   ├─→ Calls UART RX callback (if enabled)
   │   │
   │   └─→ Data moved to HAL UART internal buffer
   │
   ├─→ Returns to main loop (interrupt completes)
   │
   ↓ Next gnss_update() call
   │
   └─→ STM32Serial::read() / myHalfSerial_X::read()
       │
       ├─→ Get data from FIFO
       │
       └─→ Process in gnss_update()
```

## Timing Diagram

```
Main Loop Iteration (assuming 100ms gnss_update interval):

Time(ms)    Main Loop                            GNSS Module
─────────   ──────────────────────────────────   ──────────────────
0           gnss_update()                       
            ├─ polls UART (< 1ms)               Sending UBX packet
            ├─ caches data (< 1ms)
            └─ returns
            
5           (other code running)                
            
10          gnss_update() skipped               
            (interval not reached)              
            
100         gnss_update()                       
            ├─ polls UART                       Sending next packet
            ├─ processes new data
            └─ caches values
            
200         gnss_update()                       Sending next packet
            
1000        gnss_update()                       PVT cycle complete
            ├─ processes position data
            └─ caches position
            
[Repeat every ~1000ms for new position]
```

## Data Cache Lifecycle

```
gnss_init() [BLOCKING]
   │
   └─→ myGNSS.begin()
       ├─ setAutoPVT(true)          [Enable auto PVT]
       ├─ setMeasurementRate(1000)  [1 Hz]
       └─ setUART1Output(UBX)       [UBX packets only]
   
   ↓ Returns when module responds
   ↓ Cache initialized to default values (0)

Main Loop - Continuous polling:
   gnss_update()
   │
   ├─ Every 100ms:
   │  └─ myGNSS.checkUblox()       [Poll for new packets]
   │     └─ Update internal PVT data
   │
   ├─ Cache latest value:
   │  └─ cachedLatitude = myGNSS.getLatitude(0)
   │                              (non-blocking, maxWait=0)
   │
   └─ Returns immediately
   
   ↓ Application reads cached values anytime:
   gnss_get_latitude()             [Returns cached value immediately]
   gnss_get_longitude()            [0-2μs latency]
   gnss_get_altitude_msl()
   etc.
```

## Non-Blocking Design

```
BLOCKING SCENARIO (BAD for real-time):
────────────────────────────────────────
Loop iteration start
   │
   ├─→ gnss_update() calls checkUblox(maxWait=1000)
   │   │
   │   └─→ BLOCKED waiting for GNSS response
   │       ├─ May block 0-1000ms
   │       └─ Main loop frozen
   │
   └─→ [1000ms+ latency] Loop continues
   
This prevents other time-sensitive code from running!


NON-BLOCKING SCENARIO (GOOD - our implementation):
──────────────────────────────────────────────────
Loop iteration start (time: 0ms)
   │
   ├─→ gnss_update() calls checkUblox()
   │   │
   │   ├─ Check FIFO availability (0-50μs)
   │   ├─ Read available bytes (0-1ms)
   │   ├─ Parse packets (~1-5ms)
   │   │
   │   └─ Return immediately (1-10ms total)
   │
   ├─→ (Other code running - NOT BLOCKED)
   │
   └─→ Loop iteration complete
   
Next iteration (time: 10ms)
   │
   ├─→ (Other code)
   │
   └─→ Loop iteration complete
   
This allows main loop and other tasks to run continuously!
```

## Integration Points

```
                YOUR CODE
                   │
      ┌────────────┼────────────┐
      │            │            │
      ↓            ↓            ↓
    main()     Main Loop    Interrupts
      │            │            │
      │            │         UART3_IRQHandler
      │            │            │
      ├─ Init      ├─ calls:     ├─ HAL_UART_IRQHandler
      │  gnss_init │  gnss_update│   (auto-generated)
      │  (1-2s)    │  (10-100Hz) │
      │            │             ├─ Copies byte to buffer
      ↓            ↓             ↓
   READY        RUNNING        INTERRUPT
```

## Data Types and Formats

```
┌─────────────────────────────────────────────────────────────┐
│ Data Type and Format Used by gnss_get_*() Functions        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Latitude/Longitude:                                        │
│   Type: int32_t                                           │
│   Unit: Degrees × 10^-7                                  │
│   Range: ±180,000,000 (±180°)                           │
│   Example: 401234567 = 40.1234567°                      │
│   Convert: value / 10,000,000.0 = decimal degrees       │
│                                                            │
│ Altitude (MSL):                                           │
│   Type: int32_t                                          │
│   Unit: Millimeters                                      │
│   Range: ±2,000,000 (±2000 km)                         │
│   Example: 45321 = 45.321 meters                        │
│   Convert: value / 1,000.0 = meters                     │
│                                                            │
│ Ground Speed:                                            │
│   Type: int32_t                                         │
│   Unit: mm/s                                            │
│   Range: 0-50,000 (0-50 m/s)                          │
│   Example: 12345 = 12.345 m/s                         │
│   Convert: value / 1,000.0 = m/s                      │
│                                                           │
│ Heading:                                                │
│   Type: int32_t                                        │
│   Unit: Degrees × 10^-5                               │
│   Range: 0-3,600,000 (0-360°)                        │
│   Example: 12034567 = 120.34567°                     │
│   Convert: value / 100,000.0 = degrees               │
│                                                          │
│ SIV (Satellites In View):                             │
│   Type: uint8_t                                       │
│   Range: 0-99                                         │
│   0 = No fix, >4 = Valid 3D fix                      │
│                                                          │
│ Valid Fix:                                            │
│   Type: bool                                          │
│   Value: (getSIV() > 0)                             │
│   true = Valid fix available                         │
│                                                          │
└─────────────────────────────────────────────────────────────┘
```

## Method Call Sequence

### Typical Initialization
```
main()
  │
  ├─→ gnss_init(&huart3, &huart3_IT_ready)  [BLOCKING, ~1-2s]
  │   │
  │   ├─→ myHalfSerial_X::init() [Setup UART]
  │   │
  │   ├─→ UbloxGNSSWrapper::begin()
  │   │   │
  │   │   ├─→ myGNSS.begin(*pSerial)      [Handshake]
  │   │   │
  │   │   ├─→ myGNSS.setAutoPVT(true)     [Enable auto]
  │   │   │
  │   │   ├─→ myGNSS.setMeasurementRate() [1Hz]
  │   │   │
  │   │   └─→ myGNSS.setUART1Output()     [Configure]
  │   │
  │   └─→ Returns true/false
  │
  └─→ Ready for use
```

### Typical Main Loop
```
while (1) {
  gnss_update()                           [<10ms, every 100ms]
    │
    ├─→ Check if interval elapsed
    │
    ├─→ myGNSS.checkUblox()
    │   ├─→ Poll UART for data
    │   ├─→ Parse UBX packets
    │   └─→ Update internal data
    │
    ├─→ Cache values:
    │   ├─→ myGNSS.getLatitude(0)
    │   ├─→ myGNSS.getLongitude(0)
    │   ├─→ myGNSS.getAltitudeMSL(0)
    │   ├─→ myGNSS.getGroundSpeed(0)
    │   ├─→ myGNSS.getHeading(0)
    │   └─→ myGNSS.getSIV(0)
    │
    └─→ Exit (non-blocking)
  
  // ACCESS DATA (NON-BLOCKING):
  lat = gnss_get_latitude()               [<1μs]
  lon = gnss_get_longitude()              [<1μs]
  fix = gnss_has_valid_fix()             [<1μs]
  
  // YOUR OTHER CODE:
  // no blocking, no waiting
}
```

---

This architecture ensures:
- ✅ Non-blocking main loop
- ✅ Interrupt-driven UART
- ✅ Cached data for instant access
- ✅ Real-time safe operation

