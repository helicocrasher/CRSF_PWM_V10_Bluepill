#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include "main.h"

// Role identifiers
#define UART_ROLE_NONE 0
#define UART_ROLE_USART1 1
#define UART_ROLE_USART2 2
#define UART_ROLE_USART3 3

// Central UART role configuration
#define UART_ROLE_DEBUG UART_ROLE_USART2
#define UART_ROLE_GNSS UART_ROLE_USART3
#define UART_ROLE_CRSF UART_ROLE_USART1

// Buffer sizing per role
#define UART_DEBUG_FIFO_SIZE 256
#define UART_DEBUG_TX_BUF_SIZE 16

#define UART_GNSS_FIFO_SIZE 512
#define UART_GNSS_TX_BUF_SIZE 8

#define UART_CRSF_FIFO_SIZE 256
#define UART_CRSF_TX_BUF_SIZE 64

// UART handles provided by CubeMX
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

// Debug UART mapping
#if UART_ROLE_DEBUG == UART_ROLE_USART1
#define UART_DEBUG_HANDLE (&huart1)
#define UART_DEBUG_INSTANCE USART1
#elif UART_ROLE_DEBUG == UART_ROLE_USART2
#define UART_DEBUG_HANDLE (&huart2)
#define UART_DEBUG_INSTANCE USART2
#elif UART_ROLE_DEBUG == UART_ROLE_USART3
#define UART_DEBUG_HANDLE (&huart3)
#define UART_DEBUG_INSTANCE USART3
#else
#define UART_DEBUG_HANDLE ((UART_HandleTypeDef *)0)
#define UART_DEBUG_INSTANCE ((USART_TypeDef *)0)
#endif

// GNSS UART mapping
#if UART_ROLE_GNSS == UART_ROLE_USART1
#define UART_GNSS_HANDLE (&huart1)
#define UART_GNSS_INSTANCE USART1
#elif UART_ROLE_GNSS == UART_ROLE_USART2
#define UART_GNSS_HANDLE (&huart2)
#define UART_GNSS_INSTANCE USART2
#elif UART_ROLE_GNSS == UART_ROLE_USART3
#define UART_GNSS_HANDLE (&huart3)
#define UART_GNSS_INSTANCE USART3
#else
#define UART_GNSS_HANDLE ((UART_HandleTypeDef *)0)
#define UART_GNSS_INSTANCE ((USART_TypeDef *)0)
#endif

// CRSF UART mapping
#if UART_ROLE_CRSF == UART_ROLE_USART1
#define UART_CRSF_HANDLE (&huart1)
#define UART_CRSF_INSTANCE USART1
#elif UART_ROLE_CRSF == UART_ROLE_USART2
#define UART_CRSF_HANDLE (&huart2)
#define UART_CRSF_INSTANCE USART2
#elif UART_ROLE_CRSF == UART_ROLE_USART3
#define UART_CRSF_HANDLE (&huart3)
#define UART_CRSF_INSTANCE USART3
#else
#define UART_CRSF_HANDLE ((UART_HandleTypeDef *)0)
#define UART_CRSF_INSTANCE ((USART_TypeDef *)0)
#endif

#endif // UART_CONFIG_H
