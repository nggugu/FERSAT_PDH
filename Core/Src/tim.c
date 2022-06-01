/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* TIM2 init function */
void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  TIM_InitStruct.Prescaler = 1;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 39999;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM2, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM2);
  LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetOnePulseMode(TIM2, LL_TIM_ONEPULSEMODE_SINGLE);
  LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM2);
  /* USER CODE BEGIN TIM2_Init 2 */
  	LL_TIM_DeInit(TIM2);
    LL_TIM_SetUpdateSource(TIM2, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetOnePulseMode(TIM2, LL_TIM_ONEPULSEMODE_SINGLE);
    LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP);
  /* USER CODE END TIM2_Init 2 */

}

/* USER CODE BEGIN 1 */
//timer_init() must be called prior to first use of wait_for()
//uses TIM2, blocking call
//don't use values of interval over (2^32-1)/10
void wait_for(uint32_t interval, uint8_t unit){
	if ( interval==0 ){
		return;

	} else{
		//frequency before prescaler is 40 MHz
		if ( unit == TIM_UNIT_MS){
			LL_TIM_SetPrescaler(TIM2,(uint32_t)3999);		//10 kHz counter clock
			LL_TIM_SetAutoReload(TIM2,(uint32_t)(interval*10));

		} else {
			LL_TIM_SetPrescaler(TIM2,(uint32_t)39);			//1 MHz counter clock
			LL_TIM_SetAutoReload(TIM2,(uint32_t)interval);
		}

		LL_TIM_GenerateEvent_UPDATE(TIM2);		//write prescaler value to shadow reg.

		//timer is go
		LL_TIM_EnableCounter(TIM2);

		while( !LL_TIM_IsActiveFlag_UPDATE(TIM2) );

		LL_TIM_ClearFlag_UPDATE(TIM2);

		return;
	}
}
/* USER CODE END 1 */
