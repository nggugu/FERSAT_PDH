#ifndef INC_ADT7301_H_
#define INC_ADT7301_H_

#include "spi.h"
#include "gpio.h"

#define TEMP1_CS_GPIOx	GPIOA
#define TEMP1_CS_PIN	LL_GPIO_PIN_3
#define TEMP2_CS_GPIOx	GPIOA
#define TEMP2_CS_PIN	LL_GPIO_PIN_10
#define TEMP3_CS_GPIOx	GPIOA
#define TEMP3_CS_PIN	LL_GPIO_PIN_11

typedef struct ADT7301 {
	SPI_TypeDef *SPIx;
	float samples[3];
} ADT7301;

typedef enum {
	TEMP1 = 0,
	TEMP2 = 1,
	TEMP3 = 2
} temp_sensor;

void ADT7301_Init(ADT7301 *adt7301_struct, SPI_TypeDef *SPIx);
void ADT7301_Collect_Sample(ADT7301 *adt7301, temp_sensor ts);
void ADT7301_Shutdown(ADT7301 *adt7301, temp_sensor ts);
void ADT7301_Wakeup(ADT7301 *adt7301, temp_sensor ts);

#endif /* INC_ADT7301_H_ */
