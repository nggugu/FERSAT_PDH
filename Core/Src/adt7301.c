#include "adt7301.h"

// Initializes required memory structures and sets SPI mode.
void ADT7301_Init(ADT7301 *adt7301_struct, SPI_TypeDef *SPIx) {
	adt7301_struct->SPIx = SPIx;
	SPI_Set_Mode(CPOL1_CPHA1, SPIx);
}

// Collects a sample from one temperature sensor, specified by parameter ts.
// Also performs conversion from raw data to a floating point temperature value.
void ADT7301_Collect_Sample(ADT7301 *adt7301, temp_sensor ts) {
	uint8_t sample_bytes[2];
	uint8_t dummy_bytes[2] = {0};

	if (ts == TEMP1) {
		LL_GPIO_ResetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, dummy_bytes, sample_bytes);
		LL_GPIO_SetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
	} else if (ts == TEMP2) {
		LL_GPIO_ResetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, dummy_bytes, sample_bytes);
		LL_GPIO_SetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
	} else if (ts == TEMP3) {
		LL_GPIO_ResetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, dummy_bytes, sample_bytes);
		LL_GPIO_SetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
	}

	int16_t tmp = (sample_bytes[0] << 8) | sample_bytes[1];

	if ((tmp & (1 << 13)) != 0) {
		tmp -= 16384;
	}

	adt7301->samples[ts] = (float)tmp;
	adt7301->samples[ts] /= 32.;
}

// Puts temperature sensor ts into shutdown mode.
void ADT7301_Shutdown(ADT7301 *adt7301, temp_sensor ts) {
	uint8_t dummy_buffer[2];
	uint8_t shutdown_cmd[2] = {0x20, 0x00};

	if (ts == TEMP1) {
		LL_GPIO_ResetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, shutdown_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
	} else if (ts == TEMP2) {
		LL_GPIO_ResetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, shutdown_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
	} else if (ts == TEMP3) {
		LL_GPIO_ResetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, shutdown_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
	}
}

// Wakes sensor up from shutdown mode.
void ADT7301_Wakeup(ADT7301 *adt7301, temp_sensor ts) {
	uint8_t dummy_buffer[2];
	uint8_t wakeup_cmd[2] = {0};

	if (ts == TEMP1) {
		LL_GPIO_ResetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, wakeup_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP1_CS_GPIOx, TEMP1_CS_PIN);
	} else if (ts == TEMP2) {
		LL_GPIO_ResetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, wakeup_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP2_CS_GPIOx, TEMP2_CS_PIN);
	} else if (ts == TEMP3) {
		LL_GPIO_ResetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
		SPI_TransmitReceive(adt7301->SPIx, 2, wakeup_cmd, dummy_buffer);
		LL_GPIO_SetOutputPin(TEMP3_CS_GPIOx, TEMP3_CS_PIN);
	}
}



