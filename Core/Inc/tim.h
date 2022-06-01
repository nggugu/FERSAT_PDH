/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
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
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define TIM_UNIT_US		0
#define TIM_UNIT_MS		1
/* USER CODE END Private defines */

void MX_TIM2_Init(void);

/* USER CODE BEGIN Prototypes */
void wait_for(uint32_t interval, uint8_t wait_unit);

//approx 50 ns
__STATIC_INLINE void wait_for_50_ns(void) __attribute__((always_inline));
__STATIC_INLINE void wait_for_50_ns(void){
	//frequency before prescaler is 84 MHz
	// 0 8
	WRITE_REG(TIM2->PSC, 0);
	WRITE_REG(TIM2->ARR, 4);
	SET_BIT(TIM2->EGR, TIM_EGR_UG); //update event generation
	SET_BIT(TIM2->CR1, TIM_CR1_CEN); //counter enable
	while( !READ_BIT(TIM2->SR, TIM_SR_UIF) ); //wait for update flag
	WRITE_REG(TIM2->SR, ~(TIM_SR_UIF));	//clear update flag

	return;
}
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

