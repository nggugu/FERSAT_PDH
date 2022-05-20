/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
struct uart_buffer uart_rx = {0};
/* USER CODE END 0 */

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
  /**USART2 GPIO Configuration
  PD5   ------> USART2_TX
  PD6   ------> USART2_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),5, 0));
  NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART2);
  LL_USART_Enable(USART2);
  /* USER CODE BEGIN USART2_Init 2 */

  LL_USART_EnableIT_RXNE(USART2);

  /* USER CODE END USART2_Init 2 */

}

/* USER CODE BEGIN 1 */
void uart_send_char(uint8_t c){
	while( !READ_BIT(USART2->ISR, USART_ISR_TXE) );
	USART2->TDR = c;
	while( !READ_BIT(USART2->ISR, USART_ISR_TC) );
}

void uart_send_data(uint8_t * data, uint32_t nr_bytes){
	uint32_t i;

	for(i=0; i<nr_bytes; i++){
		while( !READ_BIT(USART2->ISR, USART_ISR_TXE) );
		USART2->TDR = *(data+i);
	}
	while( !READ_BIT(USART2->ISR, USART_ISR_TC) );

	return;
}

//retval 0 - buffer empty
//			-char received
uint32_t uart_rec_char(uint8_t *c){
	uint32_t retval=0;

	NVIC_DisableIRQ(USART2_IRQn);
	if( uart_rx.head!=uart_rx.tail ){
		uart_rx.tail = (uart_rx.tail+1)%UART_RXBUF_SIZE;
		*c = uart_rx.buffer[uart_rx.tail];
		retval = 1;
	}
	NVIC_EnableIRQ(USART2_IRQn);

	return retval;
}

void printf_eig(const char * text){
	uint16_t i;

	for( i=0; i<=255; i++){
		if( *(text+i)!='\0' ) uart_send_char((uint8_t)(*(text+i)));
		else break;
	}

};

uint8_t* gets_eig(uint8_t *s){
	uint8_t c;
	uint8_t i=0;
	while(1){
		if (uart_rec_char(&c) != 0){
			if ( c=='\n'){
				if ( i==0 ) {
					return NULL;
				} else {
					s[i]='\0';
					return s;
				}
			} else {
				s[i] = c;
				i++;
				if (i==7) {
					s[i] = '\0';
					break;
				}
			}
		}
	}
	return s;
}

uint16_t parse_file_name(uint8_t * s){
	uint16_t file_name=0, pow;
	uint8_t i, digits=0, k;


	for(i=0; i<8; i++){
		if( s[i]=='\0' ){
			digits=i;
			break;
		}
	}

	if( digits==0 || digits>4) return 0xFFFF;

	for( i=0; i<digits; i++){
		pow = 1;
		for (k=0;k<i;k++){
			pow=pow*10;
		}
		if(s[digits-1-i]<48 || s[digits-1-i]<48) return 0xFFFF;

		file_name = file_name + (s[digits-1-i]-48) * pow;
	}

	if(file_name>1023){
		return 0xFFFF;
	} else {
		return file_name;
	}
}

uint16_t parse_uint16(uint8_t * s){
	uint16_t retval=0, pow;
	uint8_t i, digits=0, k;

	for(i=0; i<6; i++){
		if( s[i]=='\0' ){
			digits=i;
			break;
		}
	}
	for( i=0; i<digits; i++){
		pow = 1;
		for (k=0;k<i;k++){
			pow=pow*10;
		}
		retval = retval + (s[digits-1-i]-48) * pow;
	}

	return retval;
}
/* USER CODE END 1 */
