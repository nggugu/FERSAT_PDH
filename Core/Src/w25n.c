#include "w25n.h"

// instruction = instruction itself (8b) | instruction additional info (8b)
// instruction additional info = D | PN[1] | PN[0] | RB | RBN[1] | RBN[0] | DO | DI
//
// D - dummy byte after instruction T/F
// PN - number of parameter bytes - 00 - 0 bytes
//									01 - 1 byte
//									10 - 2 bytes
//									11 - 4 bytes
// RB - instruction returns bytes (other than data) - T/F
// RBN - number of non data bytes returned-(if RB F don't care)- 00 - 1  byte
//		 	    												 01 - 2  bytes
// 																 10 - 3  bytes
//																 11 - 32 bytes
// DO - data out (sent to memory) T/F
// DI - data in (received from memory) T/F
const uint8_t W25N_instruction[14][2] = {
		{0xFF,0b00000000},		//w25n_reset
		{0x9F,0b10011000},		//w25n_jedec_id
		{0x0F,0b00110000},		//w25n_read_sr
		{0x1F,0b01000000},		//w25n_write_sr
		{0x06,0b00000000},		//w25n_write_enable
		{0x04,0b00000000},		//w25n_write_disable
		{0xA1,0b01100000},		//w25n_bb_manage
		{0xA5,0b10011100},      //w25n_read_bbm_lut
		{0xD8,0b11000000},		//w25n_block_erase
		{0x02,0b01000010},		//w25n_prog_data_load
		{0x84,0b01000010},		//w25n_rand_prog_data_load
		{0x10,0b11000000},		//w25n_prog_execute
		{0x13,0b11000000},		//w25n_page_data_read
		{0x03,0b01000001}		//w25n_read_data

};

void W25N_instruction_execute( W25N_instruction_nr instr, uint8_t *params, uint8_t *data_or_rbs, uint32_t nr_data_bytes){
	uint32_t nr_param_bytes = NR_PARAM_BYTES(W25N_instruction[instr][1]);
	uint32_t nr_return_bytes = NR_RETURN_BYTES(W25N_instruction[instr][1]);
	uint32_t return_bytes_tf = RETURN_BYTES_TF(W25N_instruction[instr][1]);
	uint32_t data_in_tf = DATA_IN_TF(W25N_instruction[instr][1]);
	uint32_t i;
	uint32_t dummy __attribute__((unused));

	taskENTER_CRITICAL();

	SET_BIT(W25N_SPIn->CR1, SPI_CR1_SPE); //SPI enable
	W25N_CS_LOW();
	//wait_for_50_ns();

	LL_SPI_TransmitData8(W25N_SPIn, W25N_instruction[instr][0]);


	if( DUMMY_PRESENT(W25N_instruction[instr][1]) ){			//dummy after instruction
		while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
		LL_SPI_TransmitData8(W25N_SPIn, 0x00);
		while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
		dummy = LL_SPI_ReceiveData8(W25N_SPIn);
	}

	while ( nr_param_bytes-- ){										//parameters
		while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
		LL_SPI_TransmitData8(W25N_SPIn, *params++);
		while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
		dummy = LL_SPI_ReceiveData8(W25N_SPIn);
	}

	if ( DATA_OUT_TF(W25N_instruction[instr][1]) ){					//data out
		while ( nr_data_bytes-- ){
			while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
			LL_SPI_TransmitData8(W25N_SPIn, *data_or_rbs++);
			while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
			dummy = LL_SPI_ReceiveData8(W25N_SPIn);

		}
	}

	if( return_bytes_tf ){											//return bytes
		while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
		LL_SPI_TransmitData8(W25N_SPIn, 0x00);
		while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
		dummy = LL_SPI_ReceiveData8(W25N_SPIn);
		nr_return_bytes--;

		while ( nr_return_bytes-- ){
			while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
			LL_SPI_TransmitData8(W25N_SPIn, 0x00);
			while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
			*data_or_rbs++ = LL_SPI_ReceiveData8(W25N_SPIn);

		}
	}

	if ( data_in_tf ){ 			//data in
		for( i=0; i<2; i++){
			while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
			LL_SPI_TransmitData8(W25N_SPIn, 0x00);
			while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
			dummy = LL_SPI_ReceiveData8(W25N_SPIn);
		}
		nr_data_bytes--;

		while ( nr_data_bytes-- ){
			while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
			LL_SPI_TransmitData8(W25N_SPIn, 0x00);
			while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );
			*data_or_rbs++ = LL_SPI_ReceiveData8(W25N_SPIn);
		}
	}

	while( !LL_SPI_IsActiveFlag_RXNE(W25N_SPIn) );	//final byte receive
	if ( return_bytes_tf || data_in_tf ){
		*data_or_rbs = LL_SPI_ReceiveData8(W25N_SPIn);
	} else {
		dummy = LL_SPI_ReceiveData8(W25N_SPIn);
	}

	while( !LL_SPI_IsActiveFlag_TXE(W25N_SPIn) );
	while( LL_SPI_IsActiveFlag_BSY(W25N_SPIn) );
	CLEAR_BIT(W25N_SPIn->CR1, SPI_CR1_SPE);				//disable SPI, deselect CS (NSS)
	//wait_for_50_ns();
	W25N_CS_HIGH();

	taskEXIT_CRITICAL();

	wait_for_50_ns();									//tCSH 50 ns worst case
}
