/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
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
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "ads131m08.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_SPI3_Init(void);

/* USER CODE BEGIN Prototypes */
uint32_t get_JEDEC_ID(void);

// ------ dodano za sensor board ------------------
typedef enum {
	CPOL0_CPHA1,
	CPOL1_CPHA1
} SPI_mode;

void SPI_TransmitReceive(SPI_TypeDef *SPIx, uint8_t len, uint8_t *tx_buffer, uint8_t *rx_buffer);
void SPI_Disable(SPI_TypeDef *SPIx);
void SPI_Enable_DMA_Tx_Request(SPI_TypeDef *SPIx);
void SPI_Enable_DMA_Rx_Request(SPI_TypeDef *SPIx);
void SPI_Disable_DMA_Requests(SPI_TypeDef *SPIx);
void SPI_Enable_RXNE_TXE_Interrupts(SPI_TypeDef *SPIx);
void SPI_Start_Transfer(SPI_TypeDef *SPIx);
void SPI_Set_Mode(SPI_mode mode, SPI_TypeDef *SPIx);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

