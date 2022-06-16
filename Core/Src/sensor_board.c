/*
 * sensor_board.c
 *
 *  Created on: Jun 1, 2022
 *      Author: petar
 */

#include "sensor_board.h"

float complex_samples[NUM_SAMPLES * 8 * 2] _SECTION_RAM2;

// Initializes the required memory structures for sensor board operations.
void SB_Init(Sensor_Board *sb) {
	static ADS131M08 adc;
	adc.complex_samples = complex_samples;
	sb->adc = &adc;
	static ADT7301 ts;
	sb->tmp_sensor = &ts;
}

// Starts ADC sampling procedure. This function is non-blocking, since
// sampling is handled by DMA.
void SB_Start_ADC_Sampling(Sensor_Board *sb) {
	ADC_Init(sb->adc, SB_SPIx, SB_DMAx);
	ADC_Start_Sampling(sb->adc);
}

// Fetches 24-bit ADC samples from memory buffer, performs conversion from integer to float,
// and stores samples into an array of complex floating-point numbers intended to be used as
// the source buffer for CMSIS FFT implementation.
void SB_Get_Complex_Samples(Sensor_Board *sb) {
	while (!sb->adc->sampling_complete_flag);
	uint32_t dest_index = 0;

	// One ADC frame is made of 10 24-bit words (one for each channel).
	// The first and last word should be ignored, and others truncated
	// to 16 bits.

	for (uint8_t ch = 0; ch < 8; ch++) {
		uint16_t ch_offset = 3 + ch * 3; // ignore first 3 bytes (command response)

		for (uint16_t sample = 0; sample < NUM_SAMPLES; sample++) {
			uint16_t sample_offset = sample * BYTES_PER_SAMPLE;

			uint8_t first_byte = sb->adc->samples[ch_offset + sample_offset];
			uint8_t second_byte = sb->adc->samples[ch_offset + sample_offset + 1];
			// third byte is ignored, we only want 16 bits

			int16_t adc_value = ((int16_t) first_byte << 8) | ((int16_t) second_byte);

			complex_samples[dest_index++] = ((float) adc_value / SB_ADC_MAX_VALUE_16) * SB_ADC_VREF;
			complex_samples[dest_index++] = 0;
		}
	}

// Collects samples from sensor board temperature sensors.
void SB_Get_Temperature_Readings(Sensor_Board *sb) {
	ADT7301_Init(sb->tmp_sensor, SB_SPIx);

	ADT7301_Wakeup(sb->tmp_sensor, TEMP1);
	ADT7301_Collect_Sample(sb->tmp_sensor, TEMP1);
	ADT7301_Shutdown(sb->tmp_sensor, TEMP1);

	ADT7301_Wakeup(sb->tmp_sensor, TEMP2);
	ADT7301_Collect_Sample(sb->tmp_sensor, TEMP2);
	ADT7301_Shutdown(sb->tmp_sensor, TEMP2);

//	ADT7301_Wakeup(sb->tmp_sensor, TEMP3);
//	ADT7301_Collect_Sample(sb->tmp_sensor, TEMP3);
//	ADT7301_Shutdown(sb->tmp_sensor, TEMP3);
}






