/*
 * user_main.h
 * Extracted user code declarations
 */
#ifndef USER_MAIN_H
#define USER_MAIN_H

#define TARGET_BLUEPILL
// #define TARGET_MATEKSYS_CRSF_PWM_V10

#include "main.h"
#include "platform_abstraction.h"
#include <stdint.h>
#include "stm32_arduino_compatibility.h"
#include "mySerial.h"

// C++ only includes
#ifdef __cplusplus
#include "ublox_gnss_wrapper.h"
#include "ublox_gnss_example.h"
#endif

//#include "myHalfSerial_X.h"
//#include "../SparkFun_u-blox_GNSS_Arduino_Library/src/SparkFun_u-blox_GNSS_Arduino_Library.h"

#define num_PWM_channels 10

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
//extern TIM_HandleTypeDef htim16;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern TIM_HandleTypeDef* Timer_map[num_PWM_channels];
extern unsigned int PWM_Channelmap[num_PWM_channels];


#ifdef __cplusplus
extern "C" {
#endif

void gnss_module_init(void);
void gnss_module_update(uint32_t millis_now);

void Serial_InitUART2(void);  // Initialize Serial (UART2) for debug output
// or defined in stm32_arduino_compatibility.cpp

void user_init(void);
void user_loop_step(void);
//static void user_pwm_setvalue(uint8_t pwm_channel, uint16_t PWM_pulse_lengt);
//int8_t send_UART2(char* msg); 

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* USER_MAIN_H */