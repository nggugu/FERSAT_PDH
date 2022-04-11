/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define RED_LED_ON()		LL_GPIO_SetOutputPin(LED_R_GPIO_Port,LED_R_Pin)
#define GREEN_LED_ON()		LL_GPIO_SetOutputPin(LED_G_GPIO_Port,LED_G_Pin)
#define RED_LED_OFF()		LL_GPIO_ResetOutputPin(LED_R_GPIO_Port, LED_R_Pin)
#define GREEN_LED_OFF()		LL_GPIO_ResetOutputPin(LED_G_GPIO_Port, LED_G_Pin)
/* USER CODE END Private defines */

void LED_GPIO_Init(void);
void W25N_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

