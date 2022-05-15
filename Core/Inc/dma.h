/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.h
  * @brief   This file contains all the function prototypes for
  *          the dma.c file
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
#ifndef __DMA_H__
#define __DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* DMA memory to memory transfer handles -------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define ACAM_SPIn					SPI2

#define ACAM_LL_DMA_CHANNEL_TX		LL_DMA_CHANNEL_5
#define ACAM_LL_DMA_CHANNEL_RX		LL_DMA_CHANNEL_4

#define ACAM_DMA_CHANNEL_TX_IRQn	DMA1_Channel5_IRQn
#define ACAM_DMA_CHANNEL_RX_IRQn	DMA1_Channel4_IRQn

#define ACAM_DMAn					DMA1

extern volatile uint8_t DMA_trans_complete_tx;
extern volatile uint8_t DMA_trans_complete_rx;
/* USER CODE END Private defines */

void MX_DMA_Init(void);

/* USER CODE BEGIN Prototypes */
void ACAM_DMA_Enable(void);
void ACAM_DMA_Disable(void);
void ACAM_DMA_FirstConfig_tx(void);
void ACAM_DMA_FirstConfig_rx(void);

void ACAM_DMA_config_tx(uint16_t nr_bytes, uint8_t * tx_address);
void ACAM_DMA_config_rx(uint16_t nr_bytes, uint8_t * rx_address);

__STATIC_INLINE void ACAM_DMA_ClearStatus_tx(void){
	LL_DMA_ClearFlag_HT5( ACAM_DMAn );
	LL_DMA_ClearFlag_TC5( ACAM_DMAn );
	LL_DMA_ClearFlag_TE5( ACAM_DMAn );
}

__STATIC_INLINE void ACAM_DMA_ClearStatus_rx(void){
	LL_DMA_ClearFlag_HT4( ACAM_DMAn );
	LL_DMA_ClearFlag_TC4( ACAM_DMAn );
	LL_DMA_ClearFlag_TE4( ACAM_DMAn );
}

__STATIC_INLINE void ACAM_DMA_ConfigEnable_tx(void){
	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
	while( LL_DMA_IsEnabledChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX) );

	ACAM_DMA_ClearStatus_tx();
}

__STATIC_INLINE void ACAM_DMA_ConfigEnable_rx(void){
	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);
	while( LL_DMA_IsEnabledChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX) );

	ACAM_DMA_ClearStatus_rx();
}

__STATIC_INLINE void ACAM_DMA_start_tx(void){
	LL_DMA_EnableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
}

__STATIC_INLINE void ACAM_DMA_start_rx(void){
	LL_DMA_EnableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);
}
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __DMA_H__ */

