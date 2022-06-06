/*
 * sensor_board.h
 *
 *  Created on: Jun 1, 2022
 *      Author: petar
 */

#ifndef INC_SENSOR_BOARD_H_
#define INC_SENSOR_BOARD_H_

#include "ads131m08.h"
#include "adt7301.h"

#define SB_SPIx SPI3
#define SB_DMAx DMA2

#define SB_ADC_VREF 3.3F
#define SB_ADC_MAX_VALUE_16 32768

typedef struct {
	ADS131M08 *adc;
	ADT7301 *tmp_sensor;
} Sensor_Board;

void SB_Init(Sensor_Board *sb);
void SB_Start_ADC_Sampling(Sensor_Board *sb);
void SB_Get_Complex_Samples(Sensor_Board *sb);
void SB_Get_Temperature_Readings(Sensor_Board *sb);

#endif /* INC_SENSOR_BOARD_H_ */


