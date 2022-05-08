#ifndef __W25N_H
#define __W25N_H

#include "main.h"

#ifndef NULL
#define NULL	0
#endif

#define W25N_SPIn						SPI1

#define W25N_SPI_CLOCK_ENABLE() 		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1)

//see comments in w25n.c for further info on bit assigment
#define DUMMY_PRESENT(A)		( ((A)>>7) & 0x01 ) 														//dummy y/n
#define NR_PARAM_BYTES(A)		( ( (((A)>>5) & 0x03)==0x03 ) ? 0x04 : (((A)>>5) & 0x03) )					//values 0,1,2 or 4 depending on PN[1:0]
#define RETURN_BYTES_TF(A)		( ((A)>>4) & 0x01 )															//1 if instruction receives non data bytes (some status instructions)
#define NR_RETURN_BYTES(A)		( (((A)>>3) & ((A)>>2) & 0x01) ? 0x20 : (( ((A)>>2) & 0x03 ) + 0x01) ) 	 	//values 1,2,3 or 32 depending on RBN[1:0]
#define DATA_OUT_TF(A)			( ((A)>>1) & 0x01 )															//1 if instruction sends data bytes to memory
#define DATA_IN_TF(A)			( (A) & 0x01 )																//1 if instruction receives data bytes from memory

#define W25N_CS_HIGH() 					WRITE_REG(GPIOA->BSRR, LL_GPIO_PIN_4)
#define W25N_CS_LOW() 					WRITE_REG(GPIOA->BSRR, LL_GPIO_PIN_4<<16)

typedef enum{
	w25n_reset 					= 0,
	w25n_jedec_id				= 1,
	w25n_read_sr				= 2,
	w25n_write_sr				= 3,
	w25n_write_enable			= 4,
	w25n_write_disable	  		= 5,
	w25n_bb_manage				= 6,
	w25n_read_bbm_lut			= 7,
	w25n_block_erase			= 8,
	w25n_prog_data_load 		= 9,
	w25n_rand_prog_data_load	= 10,
	w25n_prog_execute     		= 11,
	w25n_page_data_read			= 12,
	w25n_read_data				= 13

} W25N_instruction_nr;

extern const uint8_t W25N_instruction[14][2];

void W25N_instruction_execute( W25N_instruction_nr instr, uint8_t *params, uint8_t *data_or_rbs, uint32_t nr_data_bytes);

#endif
