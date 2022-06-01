#ifndef INC_ADS131M08_H_
#define INC_ADS131M08_H_

#include "spi.h"
#include "gpio.h"
#include "dma.h"

#define NUM_SAMPLES 512
#define BYTES_PER_SAMPLE 30

#define ADC_DRDY_IRQn EXTI4_IRQn

#define ADC_CS_GPIOx	GPIOA
#define ADC_CS_PIN		LL_GPIO_PIN_1
#define ADC_RST_GPIOx	GPIOA
#define ADC_RST_PIN		LL_GPIO_PIN_2
#define ADC_CLK_GPIOx	GPIOC
#define ADC_CLK_PIN		LL_GPIO_PIN_9

typedef struct {
	SPI_TypeDef *SPIx;
	DMA_TypeDef *DMAx;
	uint16_t num_samples;
	uint8_t *samples;
	volatile uint8_t sampling_complete_flag;
} ADS131M08;

void ADC_Init(ADS131M08 *adc_struct, SPI_TypeDef *SPIx, DMA_TypeDef *DMAx);
void ADC_DRDY_interrupt_handler();
void ADC_Start_Sampling(ADS131M08 *adc);

#endif /* INC_ADS131M08_H_ */
