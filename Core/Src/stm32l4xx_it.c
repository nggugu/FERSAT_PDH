/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32l4xx_it.h"
#include "FreeRTOS.h"
#include "task.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  #if (INCLUDE_xTaskGetSchedulerState == 1 )
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
  xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1 )
  }
#endif /* INCLUDE_xTaskGetSchedulerState */
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */

  /* USER CODE END EXTI4_IRQn 0 */
  if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4) != RESET)
  {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
    /* USER CODE BEGIN LL_EXTI_LINE_4 */
    ADC_DRDY_interrupt_handler();
    /* USER CODE END LL_EXTI_LINE_4 */
  }
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */
	if ( LL_DMA_IsActiveFlag_TC4(DMA1) ){
		DMA_trans_complete_rx = 1;
		LL_DMA_ClearFlag_TC4(DMA1);
	}
  /* USER CODE END DMA1_Channel4_IRQn 0 */

  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */

  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */
	if ( LL_DMA_IsActiveFlag_TC5(DMA1) ){
		DMA_trans_complete_tx = 1;
		LL_DMA_ClearFlag_TC5(DMA1);
	}
  /* USER CODE END DMA1_Channel5_IRQn 0 */

  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */

  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles I2C1 event interrupt.
  */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */
	uint32_t I2C_StatusRegister = I2C1->ISR;	// Status register

	// Transfer Register Empty
	if (READ_BIT(I2C_StatusRegister, I2C_ISR_TXIS) == I2C_ISR_TXIS) {
		// Send data according to number of bytes left to be transfered
		switch (I2C_Trans.bytesLeft) {
			case 3:
				/*
				 * bytesLeft = 3 >>>> Transmit 8 MSBs of target register
				 */
				LL_I2C_TransmitData8(I2C1, acam_I2C->HIGH);
				I2C_Trans.bytesLeft = 2;
				break;
			case 2:
				/*
				 * bytesLeft = 2 >>>> Transmit 8 MSBs of target register or transmit 8 LSBs of target register,
				 * according to the type of transfer
				 */
				LL_I2C_TransmitData8(I2C1, (I2C_Trans.type == ACAM_I2C_WRITE_REG) ? acam_I2C->LOW : acam_I2C->HIGH);
				I2C_Trans.bytesLeft = 1;
				break;
			case 1:
				/*
				 * bytesLeft = 1 >>>> Transmit 8 LSBs of target register or transmit data to be written,
				 * according to the type of transfer
				 */
				LL_I2C_TransmitData8(I2C1, (I2C_Trans.type == ACAM_I2C_WRITE_REG) ? acam_I2C->CMD : acam_I2C->LOW);
				I2C_Trans.bytesLeft = 0;
				break;
			case 0:
				/*
				 * bytesLeft = 0 >>>> Disable Transfer Register Empty interrupt
				 */
				LL_I2C_DisableIT_TX(I2C1);
				break;
			default:
				break;
		}
	}

	// Receive Register Not Empty
	if (READ_BIT(I2C_StatusRegister, I2C_ISR_RXNE) == I2C_ISR_RXNE) {
		I2C_Trans.retval = LL_I2C_ReceiveData8(I2C1);	// Read data from Receive Register
		I2C_Trans.type = I2C_COMPLETE;					// Change the status of communication
	}

	// Transfer complete
	if (READ_BIT(I2C_StatusRegister, I2C_ISR_TC) == I2C_ISR_TC) {
		I2C_Trans.status = I2C_COMPLETE;	// Change the status of communication
		LL_I2C_DisableIT_TC(I2C1);			// Disable Transfer Complete interrupt
		LL_I2C_GenerateStopCondition(I2C1);	// Stop the communication
	}
  /* USER CODE END I2C1_EV_IRQn 0 */

  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
	uint8_t rx_data;

	rx_data = LL_USART_ReceiveData8(USART2);

	if( (uart_rx.head+1)%UART_RXBUF_SIZE != uart_rx.tail ){
		//still room left
		uart_rx.head = (uart_rx.head+1)%UART_RXBUF_SIZE;
		uart_rx.buffer[uart_rx.head] = rx_data;
	}
	return;
  /* USER CODE END USART2_IRQn 0 */
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel1 global interrupt.
  */
void DMA2_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel1_IRQn 0 */
  DMA_Transfer_Complete_RX_interrupt_handler();
  /* USER CODE END DMA2_Channel1_IRQn 0 */

  /* USER CODE BEGIN DMA2_Channel1_IRQn 1 */

  /* USER CODE END DMA2_Channel1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel2 global interrupt.
  */
void DMA2_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel2_IRQn 0 */
  DMA_Transfer_Complete_TX_interrupt_handler();
  /* USER CODE END DMA2_Channel2_IRQn 0 */

  /* USER CODE BEGIN DMA2_Channel2_IRQn 1 */

  /* USER CODE END DMA2_Channel2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
