/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define UART_RXBUF_SIZE 32

struct uart_buffer{
	uint8_t buffer[UART_RXBUF_SIZE];
	uint16_t head;
	uint16_t tail;
};

extern struct uart_buffer uart_rx;
/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void uart_init(void);
void uart_send_char(uint8_t c);
void uart_send_data(uint8_t * data, uint32_t nr_bytes);
uint32_t uart_rec_char(uint8_t *c);
void printf_eig(const char * text);
uint8_t* gets_eig(uint8_t *s);

uint16_t parse_file_name(uint8_t * s);
uint16_t parse_uint16(uint8_t * s);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

