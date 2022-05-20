/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
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
#include "dma.h"

/* USER CODE BEGIN 0 */
volatile uint8_t DMA_trans_complete_tx;
volatile uint8_t DMA_trans_complete_rx;
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
  NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
  NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/* USER CODE BEGIN 2 */
void ACAM_DMA_Enable(void){
	LL_SPI_EnableDMAReq_TX( ACAM_SPIn );
	LL_SPI_EnableDMAReq_RX( ACAM_SPIn );

	LL_DMA_EnableIT_TC( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
	LL_DMA_EnableIT_TC( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);

	NVIC_ClearPendingIRQ(ACAM_DMA_CHANNEL_TX_IRQn);
	NVIC_ClearPendingIRQ(ACAM_DMA_CHANNEL_RX_IRQn);

	NVIC_SetPriority( ACAM_DMA_CHANNEL_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_SetPriority( ACAM_DMA_CHANNEL_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));

	NVIC_EnableIRQ(ACAM_DMA_CHANNEL_TX_IRQn);
	NVIC_EnableIRQ(ACAM_DMA_CHANNEL_RX_IRQn);

}

void ACAM_DMA_Disable(void){
	LL_SPI_DisableDMAReq_TX( ACAM_SPIn );
	LL_SPI_DisableDMAReq_RX( ACAM_SPIn );

	LL_DMA_DisableIT_TC( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
	LL_DMA_DisableIT_TC( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);

	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);				//theoretically, isEnabledStream should be polled, but at this point all transfers should have completed
	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);

	NVIC_DisableIRQ(ACAM_DMA_CHANNEL_TX_IRQn);
	NVIC_DisableIRQ(ACAM_DMA_CHANNEL_RX_IRQn);

}

void ACAM_DMA_FirstConfig_tx(void){
	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
	while( LL_DMA_IsEnabledChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX) );

	ACAM_DMA_ClearStatus_tx();

	LL_DMA_ConfigTransfer( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX,   LL_DMA_DIRECTION_MEMORY_TO_PERIPH \
												| LL_DMA_MODE_NORMAL \
												| LL_DMA_PERIPH_NOINCREMENT \
												| LL_DMA_MEMORY_NOINCREMENT \
												| LL_DMA_PDATAALIGN_BYTE \
												| LL_DMA_MDATAALIGN_BYTE \
												| LL_DMA_PRIORITY_HIGH);

	//LL_DMA_SetIncOffsetSize( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX, LL_DMA_OFFSETSIZE_PSIZE);
	//LL_DMA_SetChannelSelection( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX, LL_DMA_CHANNEL_0);
	//LL_DMA_DisableFifoMode( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX);
	//LL_DMA_EnableFifoMode( ACAM_DMAn, ACAM_LL_DMA_STREAM_TX);
	//LL_DMA_SetFIFOThreshold( ACAM_DMAn, ACAM_LL_DMA_STREAM_TX, LL_DMA_FIFOTHRESHOLD_1_4);

}

void ACAM_DMA_FirstConfig_rx(void){
	LL_DMA_DisableChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);
	while( LL_DMA_IsEnabledChannel( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX) );

	ACAM_DMA_ClearStatus_rx();

	LL_DMA_ConfigTransfer( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX,   LL_DMA_DIRECTION_PERIPH_TO_MEMORY \
												| LL_DMA_MODE_NORMAL \
												| LL_DMA_PERIPH_NOINCREMENT \
												| LL_DMA_MEMORY_INCREMENT \
												| LL_DMA_PDATAALIGN_BYTE \
												| LL_DMA_MDATAALIGN_BYTE \
												| LL_DMA_PRIORITY_VERYHIGH);

	LL_DMA_SetIncOffsetSize( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX, LL_DMA_OFFSETSIZE_PSIZE);
	LL_DMA_SetChannelSelection( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX, LL_DMA_CHANNEL_0);
	LL_DMA_DisableFifoMode( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX);
	//LL_DMA_EnableFifoMode( ACAM_DMAn, ACAM_LL_DMA_STREAM_RX);
	//LL_DMA_SetFIFOThreshold( ACAM_DMAn, ACAM_LL_DMA_STREAM_RX, LL_DMA_FIFOTHRESHOLD_1_4);

}

void ACAM_DMA_config_tx(uint16_t nr_bytes, uint8_t * tx_address){
	LL_DMA_SetDataLength( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX, (uint32_t)nr_bytes);
	LL_DMA_SetMemoryAddress( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX, (uint32_t)tx_address);
	LL_DMA_SetPeriphAddress( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_TX, (uint32_t)(&(SPI2->DR)));

}

void ACAM_DMA_config_rx(uint16_t nr_bytes, uint8_t * rx_address){
	LL_DMA_SetDataLength( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX, (uint32_t)nr_bytes);
	LL_DMA_SetMemoryAddress( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX, (uint32_t)rx_address);
	LL_DMA_SetPeriphAddress( ACAM_DMAn, ACAM_LL_DMA_CHANNEL_RX, (uint32_t)(&(SPI2->DR)));

}

/* USER CODE END 2 */

