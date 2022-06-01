/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_i2c.h"
#include "stm32l4xx_ll_crs.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_spi.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "cmsis_os.h"
#include "acam.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "w25n.h"
#include "usart.h"
#include "dma.h"
#include "string.h"
#include "xband.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC_DRDY_Pin LL_GPIO_PIN_4
#define ADC_DRDY_GPIO_Port GPIOC
#define ADC_DRDY_EXTI_IRQn EXTI4_IRQn
#define LED_R_Pin LL_GPIO_PIN_8
#define LED_R_GPIO_Port GPIOD
#define LED_G_Pin LL_GPIO_PIN_9
#define LED_G_GPIO_Port GPIOD
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif
/* USER CODE BEGIN Private defines */
#define XBAND_TRANSMITTER	0
#define UART_TRANSMITTER	1

#define XBAND_SELECT		UART_TRANSMITTER

#define PDH_IMG_FMT_JPG		0
#define PDH_IMG_FMT_RAW		1

#define PDH_DEVICE_OK		0
#define PDH_DEVICE_ERR		1

#define XBAND_BUF_SIZE	2040
#define SBC_BUF_SIZE	2040

typedef enum{
	dev_xband,
	dev_camera,
	dev_sensor_board
} pdh_device;

struct camera_params{
	uint16_t exp_nr_lines;
	uint16_t exp_nr_lines_frac;
	uint8_t gain;
	uint8_t format;
	uint16_t file_name;
} __PACKED;

struct xband_params{
	uint8_t reinit_filesys;
	uint16_t filename_to_transmit;
	uint8_t delete_transmited_yn;
	uint16_t filename_to_delete;
} __PACKED;

struct pdh_params{
	struct xband_params xband;
	struct camera_params camera;
} __PACKED;

struct pdh_device_status{
	pdh_device device;
	uint8_t status;
	uint32_t errno;
	uint16_t target_file_name;
} __PACKED;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
