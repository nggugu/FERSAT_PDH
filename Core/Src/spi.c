/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
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
#include "spi.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration
  PA4   ------> SPI1_NSS
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_4|LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_HARD_OUTPUT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV64;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 7;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  LL_SPI_DisableNSSPulseMgt(SPI1);
  /* USER CODE BEGIN SPI1_Init 2 */
  wait_for((uint32_t) 1000, TIM_UNIT_MS);
  /* USER CODE END SPI1_Init 2 */

}

/* USER CODE BEGIN 1 */
uint32_t get_JEDEC_ID(void){
	uint32_t out = 0, aux = 0;
	uint8_t i = 0;

	/*
	uint32_t j = 0;
	LL_SPI_Enable(SPI1);
	LL_SPI_TransmitData8(SPI1,(uint8_t)0xFF);			//reset
	while( !LL_SPI_IsActiveFlag_TXE(SPI1) );
	while( !LL_SPI_IsActiveFlag_RXNE(SPI1) );
	LL_SPI_ReceiveData8(SPI1);
	while( !LL_SPI_IsActiveFlag_TXE(SPI1) );	//wait for TX empty
	while( LL_SPI_IsActiveFlag_BSY(SPI1) );
	LL_SPI_Disable(SPI1);
	for (j=0;j<1000000;j++);
	*/
	LL_SPI_Enable(SPI1);

	LL_SPI_TransmitData8(SPI1,(uint8_t)0x9F);			//instruction transmit
	while( !LL_SPI_IsActiveFlag_TXE(SPI1) );
	LL_SPI_TransmitData8(SPI1,0x00);  			//dummy write data 2
	while( !LL_SPI_IsActiveFlag_RXNE(SPI1) );
	LL_SPI_ReceiveData8(SPI1);					//dummy read data 1

	while(!LL_SPI_IsActiveFlag_TXE(SPI1));
	LL_SPI_TransmitData8(SPI1,0x00);			//dummy write data 3
	while( !LL_SPI_IsActiveFlag_RXNE(SPI1) );
	LL_SPI_ReceiveData8(SPI1);					//dummy read data 2

	for ( i=0; i<2; i++ ){
		while( !LL_SPI_IsActiveFlag_TXE(SPI1) );
		LL_SPI_TransmitData8(SPI1,0x00);			//dummy write
		while( !LL_SPI_IsActiveFlag_RXNE(SPI1) );
		aux = ((uint32_t) LL_SPI_ReceiveData8(SPI1)) & 0x000000FF;	 			//actual read data
																				//mask used due to potential casting problems
		out = out | (aux << (i*8));
	}

	while( !LL_SPI_IsActiveFlag_RXNE(SPI1) );	//wait for final byte to arrive
	aux = ((uint32_t) LL_SPI_ReceiveData8(SPI1)) & 0x000000FF;					//actual read data
	out = out | (aux << 16);

	while( !LL_SPI_IsActiveFlag_TXE(SPI1) );	//wait for TX empty
	while( LL_SPI_IsActiveFlag_BSY(SPI1) );

	LL_SPI_Disable(SPI1);						//disable SPI to deselect CS (NSS)

	return out;
}
/* USER CODE END 1 */
