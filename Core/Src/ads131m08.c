/*
 * ads131m08.c
 *
 *  Created on: Jun 1, 2022
 *      Author: petar
 */

#include "ads131m08.h"

static void ADC_first_read(SPI_TypeDef *SPIx);
static void ADC_DMA_init();
static void ADC_Enable_SPI_DMA_transfer();
static void ADC_Set_DMA_Data_Length();

volatile uint16_t current_sample_count;
volatile uint8_t drdy_it_initialized;

uint8_t dummy_bytes[BYTES_PER_SAMPLE];

ADS131M08 *ads131m08;

// Function used to perform ADC initialization, prepares the required memory
// structures and allocates memory for samples, sets SPI mode etc. This
// function can also be used to perform initial ADC configuration if necessary.
void ADC_Init(ADS131M08 *adc_struct, SPI_TypeDef *SPIx, DMA_TypeDef *DMAx) {
	static uint8_t samples_array[NUM_SAMPLES * BYTES_PER_SAMPLE];

	adc_struct->SPIx = SPIx;
	adc_struct->DMAx = DMAx;
	adc_struct->num_samples = NUM_SAMPLES;
	adc_struct->samples = samples_array;
	adc_struct->sampling_complete_flag = 0;

	ads131m08 = adc_struct;
	SPI_Set_Mode(CPOL0_CPHA1, SPIx);
	ADC_DMA_init(adc_struct);
	current_sample_count = 0;
	drdy_it_initialized = 0;
}

// Enables DRDY interrupts and starts sample collection.
void ADC_Start_Sampling(ADS131M08 *adc) {
	ADC_first_read(adc->SPIx);
	drdy_it_initialized = 1;
	NVIC_EnableIRQ(ADC_DRDY_IRQn);
}

// Perform two sample reads to clear ADC's 2-deep FIFO buffer and ensure predictable
// DRDY pin behavior. Alternatively, a short pulse on applied to the SYNC/RESET pin
// can be used for this purpose.
static void ADC_first_read(SPI_TypeDef *SPIx) {
	uint8_t rx_buffer[30];

	LL_GPIO_ResetOutputPin(ADC_CS_GPIOx, ADC_CS_PIN); // \CS low
	SPI_TransmitReceive(SPIx, 30, dummy_bytes, rx_buffer);
	LL_GPIO_SetOutputPin(ADC_CS_GPIOx, ADC_CS_PIN); // \CS high

	for(int i = 0; i < 100; i++); // TODO: write a delay function to replace this

	LL_GPIO_ResetOutputPin(ADC_CS_GPIOx, ADC_CS_PIN); // \CS low
	SPI_TransmitReceive(SPIx, 30, dummy_bytes, rx_buffer);
	LL_GPIO_SetOutputPin(ADC_CS_GPIOx, ADC_CS_PIN); // \CS high

	SPI_Disable(SPIx);
}

// Called by DRDY interrupt routine, starts SPI DMA transfer.
void ADC_DRDY_interrupt_handler() {
	if (drdy_it_initialized && current_sample_count < NUM_SAMPLES) {
		ADC_Set_DMA_Data_Length();
		DMA_Reload_Memory_Address(ads131m08->DMAx, LL_DMA_CHANNEL_1, ads131m08->samples + current_sample_count++ * BYTES_PER_SAMPLE);
		ADC_Enable_SPI_DMA_transfer();

		NVIC_DisableIRQ(ADC_DRDY_IRQn); // Interrupt is re-enabled in DMA transfer complete routine

		SPI_Start_Transfer(ads131m08->SPIx);
	} else if (current_sample_count >= NUM_SAMPLES) {
		NVIC_DisableIRQ(ADC_DRDY_IRQn);
		ads131m08->sampling_complete_flag = 1;
		drdy_it_initialized = 0;
	}
}

static void ADC_DMA_init(ADS131M08 *adc) {
	DMA_Channel_Init(adc->DMAx, LL_DMA_CHANNEL_1, LL_SPI_DMA_GetRegAddr(ads131m08->SPIx), ads131m08->samples);
	DMA_Channel_Init(adc->DMAx, LL_DMA_CHANNEL_2, LL_SPI_DMA_GetRegAddr(ads131m08->SPIx), dummy_bytes);
}

static void ADC_Set_DMA_Data_Length() {
	DMA_Set_Channel_Data_Length(ads131m08->DMAx, LL_DMA_CHANNEL_1, BYTES_PER_SAMPLE);
	DMA_Set_Channel_Data_Length(ads131m08->DMAx, LL_DMA_CHANNEL_2, BYTES_PER_SAMPLE);
}

static void ADC_Enable_SPI_DMA_transfer() {
	SPI_Enable_DMA_Rx_Request(ads131m08->SPIx);
	DMA_Enable_CH1_CH2(ads131m08->DMAx);
	SPI_Enable_DMA_Tx_Request(ads131m08->SPIx);
	SPI_Enable_RXNE_TXE_Interrupts(ads131m08->SPIx);
}



