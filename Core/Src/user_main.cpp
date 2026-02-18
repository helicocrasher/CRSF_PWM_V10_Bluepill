#ifndef USER_MAIN_CPP_H
#define USER_MAIN_CPP_H

#include "user_main.h"
#include "mySerial.h"
#include "ublox_gnss_example.h"
//#include <cstddef>

#include <cstddef>
#include <stdint.h>
#include <stdio.h>
#include "../AlfredoCRSF/src/AlfredoCRSF.h"
#include "platform_abstraction.h"


//#include "stm32g0xx_hal_adc.h"
#include "../SPL06-001/SPL06-001.h"
#include "../SPL06-001/spl06_001_glue.h" 



#define CRSF_BATTERY_SENSOR_CELLS_MAX 12
#define BAT_ADC_Oversampling_Ratio 16  // Must match ADC oversampling ratio
#define BAT_ADC_Voltage_divider 11.0f // Voltage divider ratio 10k and 1k resistors
#define DBG_STRINGBUFSIZE 100
char debug_str_buffer[DBG_STRINGBUFSIZE];

#ifdef TARGET_MATEKSYS_CRSF_PWM_V10
// PWM Channel to Timer and Channel mapping for the used "Matek CRSF_PWM_V10" board
TIM_HandleTypeDef* Timer_map[num_PWM_channels]={&htim2,      &htim2,       &htim16,      &htim2,       &htim3,       &htim3,       &htim3,       &htim3,       &htim1,       &htim1};
unsigned int PWM_Channelmap[num_PWM_channels]={TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_1,TIM_CHANNEL_3,TIM_CHANNEL_4,TIM_CHANNEL_3,TIM_CHANNEL_2,TIM_CHANNEL_1,TIM_CHANNEL_1,TIM_CHANNEL_2};
//   Servo Channel number                             1                 2                 3                 4                 5                 6                 7                 8                 9                 10  
//   Timer channel offset = TIM_Channel_X -1)*4       0                 4                 0                 8                12                 8                 4                 0                 0                  4                
#endif

#ifdef TARGET_BLUEPILL // BluePill or other custom board - adjust Timer_map and PWM_Channelmap according to your wiring and timers used
// PWM Channel to Timer and Channel mapping  for the used "BluePill" board
TIM_HandleTypeDef* Timer_map[num_PWM_channels]={&htim2,       &htim2,       &htim3,       &htim3,       &htim3,       &htim3,       &htim1,       &htim1,       &htim1,       &htim1};
unsigned int PWM_Channelmap[num_PWM_channels]={ TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4,TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4};
//   Servo Channel number                             1                 2                 3                 4                 5                 6                 7                 8                 9                 10  
#endif


#ifdef __cplusplus
extern "C" {

#endif

void gnss_module_init(void);
void gnss_module_update(uint32_t millis_now);


#define TARGET_MATEKSYS_CRSF_PWM_V10
TwoWire BaroWire(SDA2, SCL2);	
SPL06 BaroSensor(&BaroWire); 
//mySerial serial2, gnssSerial;
void setupBaroSensor();  
void baroProcessingTask(uint32_t millis_now);
void baroSerialDisplayTask(uint32_t millis_now);
void telemetrySendBaroAltitude(float altitude);
void telemetrySendVario( float verticalspd);


/*
extern "C" int serial2_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    serial2.write(&c, 1);
    return ch;
}
*/

//user_loop tasks - timed - prototype declarations
static void CRSF_reception_watchdog_task(uint32_t actual_millis);
static void pwm_update_task(uint32_t actual_millis);
static void LED_and_debugSerial_task(uint32_t actual_millis);
static void analog_measurement_task(uint32_t actual_millis);
static void telemetry_transmission_task(uint32_t actual_millis);
void gnssUpdateTask(uint32_t actual_millis);
void gnssDisplayTask(uint32_t actual_millis) ;


static void error_handling_task(void); 

// basic functions
static void sendCellVoltage(uint8_t cellId, float voltage);
static void user_pwm_setvalue(uint8_t pwm_channel, uint16_t PWM_pulse_length);
int8_t send_UART2(void);

extern ADC_HandleTypeDef hadc1;
mySerial serial2, gnssSerial;
STM32Stream* crsfSerial = nullptr;  // Will be initialized in user_init()
AlfredoCRSF crsf;
volatile bool ready_TX_UART2 = 1;
volatile bool ready_RX_UART2 = 1;

volatile bool ready_TX_UART3 = 1;
volatile bool ready_RX_UART3 = 1;
volatile uint8_t ready_RX_UART1 = 1;
volatile bool ready_TX_UART1 = 1;
volatile bool isCRSFLinkUp = false;
volatile uint32_t RX1_overrun = 0, crsfSerialRestartRX_counter=0, main_loop_cnt=0, ADC_period=0;
volatile uint32_t ELRS_TX_count = 0, ADC_count=0;
volatile uint16_t ADC_buffer[2]; // ADC buffer for DMA
volatile uint8_t isADCFinished=0;
volatile uint8_t i2cWriteComplete=1;
static float bat_voltage=0.0f, bat_current=0.0f;

static const float IIR_ALPHA  = 0.135755f;  // 0.5Hz cut off (fc/40 / fc=Hz /Tc=320ms)
static const float IIR_BETA  = (1.0f - IIR_ALPHA);

static float GND_altitude=0;
static float previous_alt_ASL=0, baroAltitude=0, baroTemperature=0, baroPressure=0;
static float vario=0,filt_alt_AGL=0;
static double filt_vario=0, filt_alt_ASL=0;
static uint32_t GND_alt_count=0;

// GNSS module state variables
static bool gnss_initialized = false;
static uint32_t gnss_last_update_time = 0;
// static const uint32_t GNSS_UPDATE_INTERVAL_MS = 100;  // Update every 100ms (10Hz) - reserved for future use
static uint32_t gnss_last_print_time = 0;
// static const uint32_t GNSS_PRINT_INTERVAL_MS = 2000;  // Print every 2 seconds - reserved for future use


void user_init(void)  // same as the "arduino setup()" function
{
  HAL_Delay(5);
  serial2.init(&huart2, (bool*)&ready_TX_UART2, (bool*)&ready_RX_UART2,256, 16  );
  crsfSerial = new STM32Stream(&huart1); // Ensure UART is initialized before creating STM32Stream
  crsf.begin(*crsfSerial);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_Delay(20);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC_buffer, 2);
//  setupBaroSensor();
  gnss_module_init();
}


void user_loop_step(void) // same as the "arduino loop()" function
{
  static uint32_t  actual_millis=0;

  actual_millis = HAL_GetTick();
  crsf.update();
  CRSF_reception_watchdog_task(actual_millis);
  pwm_update_task(actual_millis);
  LED_and_debugSerial_task(actual_millis);
  analog_measurement_task(actual_millis);
  baroProcessingTask(actual_millis);
//  baroSerialDisplayTask(actual_millis);
  telemetry_transmission_task(actual_millis);
  error_handling_task();
//  serial2.updateSerial();
  main_loop_cnt++;
//  HAL_Delay(2)  ; // Small delay to prevent CPU hogging - adjust as needed for timing
}

void gnssUpdateTask(uint32_t actual_millis) {
  static uint32_t last_update_time = 0;
  if ((actual_millis - last_update_time) <100) return; // Update every 100ms

  last_update_time = actual_millis;
  if (gnss_initialized) {
    gnss_update();
  }
}

void gnssDisplayTask(uint32_t actual_millis) {
  static uint32_t last_print_time = 0;
  if ((actual_millis - last_print_time) <2000) return; // Print every 2 seconds

  last_print_time = actual_millis;
  if (gnss_initialized) {
    // Print GNSS data to Serial for debugging
   // printf("GNSS Data: Lat=%.6f, Lon=%.6f, Alt=%.2f, Sats=%d\n", pGNSS->getLatitude(), pGNSS->getLongitude(), pGNSS->getAltitude(), pGNSS->getSIV());
  }
} 

static void analog_measurement_task(uint32_t actual_millis) {
  // analog measurement task running with ADC speed is fed by DMA ~22ms with oversampling 16, 
  // ADC clock is 32MHz prescaler 128, sample time 160.5 cycles Total conversion time per channel = 160.5 + 12.5 =173 cycles
  //
  static uint32_t last_adc_millis = 0;

  static float adcValue1 =0, adcValue2=0;
  static const float VOLTAGE_DIV = 11.0f;
  static const float SHUNT_RESISTOR_OHMS = 0.066f;
  static const float ADC_GAIN = 1.01f;
  static const float VOLTAGE_OFFSET = 0.00f;
  static const float CURRENT_OFFSET = 0.015f;
  static const float IIR_ALPHA = 0.239057f; // IIR filter alpha coefficient (for Sample_frequency/20 cutoff)
  static const float IIR_BETA = 1.0f - IIR_ALPHA;

  if (isADCFinished == 0) return; // Previous ADC conversion not finished
  ADC_period=actual_millis - last_adc_millis;
  last_adc_millis = actual_millis;
  isADCFinished = 0;
  adcValue1 = adcValue1*IIR_BETA + (float)ADC_buffer[0] * IIR_ALPHA; // Filter the first ADC channel (VBAT);
  adcValue2 = adcValue2*IIR_BETA + (float)ADC_buffer[1] * IIR_ALPHA; // Filter the second ADC channel (Current);
  bat_voltage = ((float)adcValue1 * 3.3f / 4095.0f * VOLTAGE_DIV * ADC_GAIN/(float)BAT_ADC_Oversampling_Ratio)-VOLTAGE_OFFSET;
  bat_current = ((float)adcValue2 * 3.3f / 4095.0f * ADC_GAIN/(float)BAT_ADC_Oversampling_Ratio / SHUNT_RESISTOR_OHMS)-CURRENT_OFFSET; 
}


static void CRSF_reception_watchdog_task(uint32_t actual_millis) {

  static  uint32_t last_watchdog_millis = 0;
  if (crsf.isLinkUp()) {last_watchdog_millis = actual_millis;  return; } // Link is up, reset watchdog timer - nothing to do
  if (actual_millis-last_watchdog_millis <10) return; // Wait for 10ms of no link or last restart attempt
  // No link for more than 10ms - restart CRSF UART RX
  last_watchdog_millis = actual_millis;
  crsfSerial->restartUARTRX(&huart1);
  crsfSerialRestartRX_counter++;
}


static void telemetry_transmission_task(uint32_t actual_millis) {
  static uint32_t last_telemetry_millis = 0;
  static uint32_t telemetry_carousel = 0;
  #define CAROUSEL_MAX 4

  if (actual_millis - last_telemetry_millis < 500/CAROUSEL_MAX) return;
  if(bat_voltage<0.0f) bat_voltage=0.0f;
  if(bat_current<0.0f) bat_current=0.0f;
  if (telemetry_carousel ==0)   sendCellVoltage(1, bat_voltage);
  if (telemetry_carousel ==1)   sendCellVoltage(2, bat_current);
  if (telemetry_carousel ==2)   telemetrySendBaroAltitude(filt_alt_AGL);
  if (telemetry_carousel ==3)   telemetrySendVario( filt_vario);
  last_telemetry_millis = actual_millis;
  telemetry_carousel++;
  telemetry_carousel %= CAROUSEL_MAX;
}

static void pwm_update_task(uint32_t actual_millis) {

  if (!isCRSFLinkUp && crsf.isLinkUp()) { // Link just came up - initialize PWM outputs
    isCRSFLinkUp = true;
    for (uint8_t channel=0; channel<num_PWM_channels; channel++){ // start up all PWMs & outouts
      HAL_TIM_PWM_Start(Timer_map[channel], PWM_Channelmap[channel]);
    }
    return;
  } // set PWM values from CRSF to PWM channel
  static uint32_t servo_update_millis =0; 
  if (actual_millis-servo_update_millis <1) return; // Update every ms to minimize delay between CRSF reception and PWM output
  servo_update_millis = actual_millis;
  for (uint8_t channel=0; channel<num_PWM_channels; channel++){
    uint16_t PWM_value = crsf.getChannel(channel+1);
    user_pwm_setvalue(channel, PWM_value);      
  }
}

static void LED_and_debugSerial_task(uint32_t actual_millis) {

  static uint32_t last_debugTerm_millis=0;
  uint16_t ch1 = crsf.getChannel(1);
  uint16_t ch2 = crsf.getChannel(2);

  if (actual_millis - last_debugTerm_millis < 200) return;
  last_debugTerm_millis = actual_millis;
//  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_14 );        //TARGET_MATEK
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2 ); // TARGET_BluePill
  
  printf("%7lu : ELRS_UP = %1d  / CH1 = %4d CH2 =  %4d, Restart = %4lu ADC_period = %4lu\r\n", (unsigned long)main_loop_cnt, crsf.isLinkUp(), ch1, ch2, (unsigned long)crsfSerialRestartRX_counter, (unsigned long)ADC_period);
}

static void error_handling_task(void) {
  // Placeholder for future error handling tasks 
  __NOP();  
}


static void sendCellVoltage(uint8_t cellId, float voltage) {
  static  uint8_t payload[3];
  
  if (cellId < 1 || cellId > CRSF_BATTERY_SENSOR_CELLS_MAX)     return;
  payload[0] = cellId;
  uint16_t voltage_be = htobe16((uint16_t)(voltage * 1000.0)); //mV
  memcpy(&payload[1], &voltage_be, sizeof(voltage_be));
  crsf.queuePacket(CRSF_SYNC_BYTE, 0x0e, payload, sizeof(payload));
}

static void user_pwm_setvalue(uint8_t pwm_channel, uint16_t PWM_pulse_length) {
  static TIM_OC_InitTypeDef sConfigOC={0,0,0,0,0,0,0};

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  uint32_t count = __HAL_TIM_GET_COUNTER(Timer_map[pwm_channel]);
  if ((count<2250) || (count >19950)) return; // do not update PWM value in the middle of a PWM pulse
  if (PWM_pulse_length <750) PWM_pulse_length =750;
  if (PWM_pulse_length >2250) PWM_pulse_length =2250;
  sConfigOC.Pulse = PWM_pulse_length;
  HAL_TIM_PWM_ConfigChannel(Timer_map[pwm_channel], &sConfigOC, PWM_Channelmap[pwm_channel]);
}


void setupBaroSensor(){   // SPL06-001 sensor version 
  unsigned status;

  BaroWire.begin();

  HAL_Delay(10);
  
  status = BaroSensor.begin(SPL06_ADDRESS_ALT,SPL06_PRODID);
  //unsigned status = BaroSensor.begin(SPL06_ADDRESS_ALT);
  if (!status) {
    printf("Could not find a valid SPL06-001 sensor, check wiring or try a different address!\\n\r");
    {
      
      printf("SensorID was:   0x%02X", BaroSensor.sensorID());
    }
    printf("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085");
    while (1) HAL_Delay(10);
  }
  BaroSensor.setSampling(SPL06::MODE_BACKGND_BOTH,
                          SPL06::SAMPLING_X16,
                          SPL06::SAMPLING_X16,
                          SPL06::RATE_X16,
                          SPL06::RATE_X16);
  printf("SPL06-001 ready");
}


void telemetrySendBaroAltitude(float altitude)
{
  crsf_sensor_baro_altitude_t crsfBaroAltitude = { 0 , 0 };

  // Values are MSB first (BigEndian)
  crsfBaroAltitude.altitude = htobe16((uint16_t)(altitude*10.0 + 10000.0));
  //crsfBaroAltitude.verticalspd = htobe16((int16_t)(verticalspd*100.0)); //TODO: fix verticalspd in BaroAlt packets
  crsf.queuePacket(CRSF_SYNC_BYTE, CRSF_FRAMETYPE_BARO_ALTITUDE, &crsfBaroAltitude, sizeof(crsfBaroAltitude) - 2);
  
}

void telemetrySendVario( float verticalspd)
{
  crsf_sensor_vario_t crsfVario = { 0 };

  // Values are MSB first (BigEndian)
  crsfVario.verticalspd = htobe16((int16_t)(verticalspd*100.0));
  crsf.queuePacket(CRSF_SYNC_BYTE, CRSF_FRAMETYPE_VARIO, &crsfVario, sizeof(crsfVario));
}


void floatToString( char* buffer, size_t bufferSize,float value) {
   int32_t intValue = (int32_t)(value * 100.0 + 0.5); // Scale to preserve two decimal places
   if (intValue > 999999) intValue = 999999; // Cap to max displayable value
   if (intValue < -99999) intValue = -99999; // Cap to min displayable value
   if(intValue < 0)     snprintf(buffer, bufferSize, "%06ld", (long int)intValue);
   else                 snprintf(buffer, bufferSize, "%6ld" , (long int)intValue);
    buffer[7] = '\0'; // Ensure null termination
    buffer[6] = buffer[5]; // Move last digit to position 6
    buffer[5] = buffer[4]; // Move second last digit to position 5
    buffer[4] = '.'; // Insert decimal point at position 4
    if(buffer[5] == ' ') buffer[5] = '0'; // Replace space with '0' if needed
    if(buffer[3] == ' ') buffer[3] = '0'; // Replace space with '0' if needed
}


void baroSerialDisplayTask(uint32_t millis_now){
  static uint32_t last_millis=0;
  if (millis_now - last_millis < 250) return;
  last_millis = millis_now;
  
  static char float_string_buffer[8];

  floatToString(float_string_buffer, sizeof(float_string_buffer), baroTemperature);
  printf("Temperature = %s*C\n\r", float_string_buffer);
  floatToString(float_string_buffer, sizeof(float_string_buffer),filt_alt_AGL);
  printf("Approx altitude ASL = %3dm ; Altitude AGL = %sm\n\r", (int)filt_alt_ASL, float_string_buffer);
  floatToString(float_string_buffer, sizeof(float_string_buffer), filt_vario);
  printf("Vario = %sm/s\n\r", float_string_buffer);
  serial2.write((uint8_t*)"\n\r", 2);
}

void baroProcessingTask(uint32_t millis_now){

  static uint32_t last_millis=0,loop_counter=0;
  if (millis_now - last_millis < 125) return;
  last_millis = millis_now;
  static uint32_t millis_start=0, millis_end=0;


  millis_start=HAL_GetTick();

  baroAltitude = BaroSensor.readPressureAltitudeMeter(1013.25);
  baroTemperature = BaroSensor.readTemperature();
  baroPressure = BaroSensor.readPressure();

  if (loop_counter <1000) {    // initialisation --> average ground altitude
    GND_altitude += baroAltitude;
    GND_alt_count++;
    filt_alt_ASL = baroAltitude;
  } else {
    filt_alt_ASL = (IIR_ALPHA * baroAltitude) + (IIR_BETA * filt_alt_ASL);
  }
  filt_alt_AGL =filt_alt_ASL - (GND_altitude / GND_alt_count);
  vario=(filt_alt_AGL-previous_alt_ASL)*20;
  filt_vario = (IIR_ALPHA * vario) + (IIR_BETA * filt_vario);
  previous_alt_ASL = filt_alt_AGL;
  loop_counter++;
  millis_end=HAL_GetTick();
  volatile uint32_t baro_processing_millis = millis_end - millis_start; // For debugging - check how long baro processing takes and if it causes delays in PWM output or CRSF reception
  printf("Baro processing time: %lu ms\n\r", (unsigned long)baro_processing_millis);
}

void gnss_module_init(void) {
  printf("\n>>> Initializing GNSS Module...\n\r");
    
    // This call blocks for ~1-2 seconds while waiting for module response
    // Safe to call during initialization
    gnss_initialized = gnss_init(&huart3,(bool*) &ready_TX_UART3, (bool*)&ready_RX_UART3);
    if (gnss_initialized) {
      printf(">>> GNSS Module: INITIALIZED OK\n\r");
      printf(">>> Waiting for satellite fix (may take 30-60 seconds on cold start)...\n\r");
      gnss_last_update_time = millis();
      gnss_last_print_time = millis();
    } 
    else {
      printf( ">>> GNSS Module: INITIALIZATION FAILED\r\n");
      printf( ">>> Check:\r\n");
      printf( ">>>  1. UART3 is enabled in STM32CubeMX\r\n");
      printf( ">>>  2. Baud rate matches module (38400 or 115200)\r\n");
      printf( ">>>  3. GNSS module is powered and connected\r\n");
      printf( ">>>  4. RX antenna is connected\r\n\r\n");
    }
    delay(1000); // Small delay to ensure UART messages are sent before any further processing
    delay(1 );
}


#ifdef __cplusplus
}  // extern "C"
#endif

#endif // USER_MAIN_CPP_H