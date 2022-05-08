#include "w25n_api.h"

const uint8_t W25N_sr_address[3] = { 0xA0, 0xB0, 0xC0};

//initializes the interface towards the W25N memory (GPIO,SPI,timers)
//ret val: 	0 - SPI communication fault
//			1 - all ok
uint8_t W25N_init(void){
	if ( (W25N_jedec_id() & 0x00FFFFFF)!=0x0021AAEF ) return 0;

	W25N_disable_block_protect();
	while(  W25N_block_protect_status() );
	return 1;
}

//deinitializes the interface towards the W25N memory
void W25N_deinit(void){
	LL_SPI_DeInit( W25N_SPIn );
}

//reads status of a bit (bits) in given status register
//input params: srn - status register number, mask - bit mask
//ret val:	0 - bit(s) not set
//			1 - bit(s) set
uint8_t W25N_check_status(W25N_SRn srn, uint8_t mask){
	if ( (W25N_read_sr(srn) & mask) == 0x00 ) return 0;
	else return 1;
}

//disables block write protection for entire memory array
//this function must be called after memory module reset, unless block protect
//bits are OTP locked (see W25N01 user manual section 7.1)
//use W25N_block_protect_status() to check if successful
void W25N_disable_block_protect(void){
	uint8_t sr1val;
	sr1val = W25N_read_sr(W25N_SR1);
	sr1val = sr1val & (~W25N_SR1_BP_MASK);
	W25N_write_sr(W25N_SR1, sr1val);

}

//enter OTP access mode
//use W25N_check_OTP_mode to check if successful
void W25N_enter_OTP_mode(void){
	uint8_t sr2val;
	sr2val = W25N_read_sr(W25N_SR2);
	sr2val = sr2val | W25N_SR2_OTP_E_MASK;
	W25N_write_sr(W25N_SR2, sr2val);

}

//exit OTP access mode
//use W25N_check_OTP_mode to check if successful
void W25N_exit_OTP_mode(void){
	uint8_t sr2val;
	sr2val = W25N_read_sr(W25N_SR2);
	sr2val = sr2val & (~W25N_SR2_OTP_E_MASK);
	W25N_write_sr(W25N_SR2, sr2val);

}

//set OTP lock bit
//note: once this bit is set, certain features cannot be changed. use with care.
//refer to user manual for more info.
//use W25N_check_OTP_lock to check if successful
void W25N_set_OTP_lock(void){
	uint8_t sr2val;
	sr2val = W25N_read_sr(W25N_SR2);
	sr2val = sr2val | W25N_SR2_OTP_L_MASK;
	W25N_write_sr(W25N_SR2, sr2val);

}

//enter OTP access mode
//use W25N_check_OTP_mode to check if successful
void W25N_enter_BUF_mode(void){
	uint8_t sr2val;
	sr2val = W25N_read_sr(W25N_SR2);
	sr2val = sr2val | W25N_SR2_BUF_MASK;
	W25N_write_sr(W25N_SR2, sr2val);

}

//exit buffer read mode
//use W25N_check_BUF_mode to check if successful
void W25N_exit_BUF_mode(void){
	uint8_t sr2val;
	sr2val = W25N_read_sr(W25N_SR2);
	sr2val = sr2val & (~W25N_SR2_BUF_MASK);
	W25N_write_sr(W25N_SR2, sr2val);

}

//reads value of status register SR1, SR2 or SR3
//input params: srn - status register number
//ret val: status register value
uint8_t W25N_read_sr(W25N_SRn srn){
	uint8_t srval;
	W25N_instruction_execute( w25n_read_sr, (uint8_t *)(&W25N_sr_address[srn-1]), &srval, 0);
	return srval;

}

//writes given value to given status register
//input params: srn - status register number, stval - status register value
//ret val: none
void W25N_write_sr(W25N_SRn srn, uint8_t srval){
	uint8_t params[2];
	params[0] = W25N_sr_address[srn-1];
	params[1] = srval;
	W25N_instruction_execute( w25n_write_sr, params, NULL, 0);

}

//issues request to reset W25N memory module. If the module isn't busy, reset will be
//executed. Then, a blocking delay of 750 us is executed, to allow for the reset
//sequence to be completed.
//ret val:	- W25N_ERR_BUSY - memory module busy, reset not issued
//			- W25N_INSTR_ISSUED - reset successful
uint8_t W25N_reset(void){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
 	} else {
 		W25N_instruction_execute( w25n_reset, NULL, NULL, 0);
 		wait_for(750,TIM_UNIT_US);
 		return W25N_INSTR_ISSUED;
 	}
}

//returns device JEDEC ID: 0xXX21AAEF, where X is don't care
//note: JEDEC ID is actualy EFAA21, but since the processor
//uses little endian, byte order is reversed
//ret val: JEDEC ID
uint32_t W25N_jedec_id(void){
	uint32_t jedec_id;
	W25N_instruction_execute( w25n_jedec_id, NULL, (uint8_t *)(&jedec_id), 0);
	return jedec_id;
}

//executes bad block manage instruction. Write enable included in function.
//note: BUSY flag should be monitored afterwards to ensure completion
//input params: lba - logical block address, pba - physical block address
//ret val: none
void W25N_bb_manage(uint16_t lba, uint16_t pba){
	uint8_t params[4];
	//reverse byte order due to endianess
	params[0] = ISOLATE_HI_BYTE(lba);
	params[1] = ISOLATE_LO_BYTE(lba);
	params[2] = ISOLATE_HI_BYTE(pba);
	params[3] = ISOLATE_LO_BYTE(pba);

	W25N_write_enable();
	W25N_instruction_execute( w25n_bb_manage, (uint8_t *)params, NULL, 0);

}

//reads BBM LUT giving out 20 LBA-PBA links (for more information
//refer to 8.2.7. section of W25N01 user manual)
//input params: - container	- pointer to a 80 byte array (20 links x 2 addresses x 2 bytes)
//							- data shape: {LBA0_l,LBA0_h,PBA0_l,PBA0_h,LBA1_l,LBA1_h,...}
//							- WARNING - a size of 80 bytes minimally must be ensured
//note: function will execute byte reordering to ensure little endian output
void W25N_read_bbm_lut(uint8_t * container){
	uint8_t i, aux;

	W25N_instruction_execute( w25n_read_bbm_lut, NULL, container, 0);

	for (i=0; i<79; i=i+2){
		aux = container[i];
		container[i] = container[i+1];
		container[i+1] = aux;
	}

}

//performs 128 kB block erase (all values become 0xFF). Write enable included in function.
//note:	- BUSY flag should be monitored afterwards to ensure completion
//		- EFAIL flag should be monitored to ensure operation success (failure indicative
//			of bad block or other condition according to W25N01 user manual section 7.3.5)
//input params: - block_nr - 128 kB block number (address) - element of [0,1023]
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_block_erase(uint16_t block_nr){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( block_nr<<0x06 );
		params[1] = ISOLATE_LO_BYTE( block_nr<<0x06 );

		W25N_write_enable();
		W25N_instruction_execute( w25n_block_erase, params, NULL, 0);

		return W25N_INSTR_ISSUED;
	}
}

//loads data into the data buffer, unused bytes will be reset to 0xFF.
//Write enable included in function.
//input params: - col_addr (byte address) - interval of [0,2047]
//				- data - pointer to the beginning of data to be transfered from local
//						storage to memory
//				- nr_bytes - number of bytes of data to be transfered (must be such
//								that the final byte does not cross 2048 byte boundary,
//								otherwise ECC parity bits corruption might occur)
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_prog_data_load(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( col_addr );
		params[1] = ISOLATE_LO_BYTE( col_addr );

		W25N_write_enable();
		W25N_instruction_execute( w25n_prog_data_load, params, data, nr_bytes);

		return W25N_INSTR_ISSUED;
	}
}

//loads data into the data buffer, unused bytes will retain their previous value from
//the data buffer.
//Write enable included in function.
//input params: - col_addr (byte address) - interval of [0,2047]
//				- data - pointer to the beginning of data to be transfered from local
//						storage to memory
//				- nr_bytes - number of bytes of data to be transfered (must be such
//								that the final byte does not cross 2048 byte boundary,
//								otherwise ECC parity bit corruption might occur)
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_rand_prog_data_load(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( col_addr );
		params[1] = ISOLATE_LO_BYTE( col_addr );

		W25N_write_enable();
		W25N_instruction_execute( w25n_rand_prog_data_load, params, data, nr_bytes);

		return W25N_INSTR_ISSUED;
	}
}

//writes data buffer to given page in memory
//Write enable included in function.
//note:	- BUSY flag should be monitored afterwards to ensure completion
//		- PFAIL flag should be monitored to ensure operation success (failure indicative
//			of bad block or other condition according to W25N01 user manual section 7.3.5)
//input params:	- page_addr - page address within memory array (entire)
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_prog_execute(uint16_t page_addr){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( page_addr );
		params[1] = ISOLATE_LO_BYTE( page_addr );

		W25N_write_enable();
		W25N_instruction_execute( w25n_prog_execute, params, NULL, 0);

		return W25N_INSTR_ISSUED;
	}
}

//reads page from memory array and transfers it into the data buffer
//note:	- BUSY flag should be monitored afterwards to ensure completion
//input params:	- page_addr - page address within memory array (entire)
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_page_data_read(uint16_t page_addr){
	if (W25N_check_busy()){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( page_addr );
		params[1] = ISOLATE_LO_BYTE( page_addr );

		W25N_instruction_execute( w25n_page_data_read, params, NULL, 0);

		return W25N_INSTR_ISSUED;
	}
}

//reads data from data buffer and sends them to the processor
//input params: - col_addr (byte address) - interval of [0,2047]
//				- data - pointer to area in local memory where data will be written
//				- nr_bytes - number of bytes of data to be transfered (must be such
//								that the final byte does not cross 2048 byte boundary,
//								otherwise irrelevant data will be transfered)
//ret val:	- W25N_ERR_BUSY - memory module busy, instruction not issued
//			- W25N_INSTR_ISSUED - instruction issuing successful
uint8_t W25N_read_data(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes){
	if( W25N_check_busy() ){
		return W25N_ERR_BUSY;
	} else {
		uint8_t params[2];
		params[0] = ISOLATE_HI_BYTE( col_addr );
		params[1] = ISOLATE_LO_BYTE( col_addr );

		W25N_instruction_execute( w25n_read_data, params, data, nr_bytes);

		return W25N_INSTR_ISSUED;
	}
}

//scan memory module for factory marked bad blocks
//input params: - BBBT - pointer to bad block bit table
void W25N_scan_factory_BB(uint32_t * BBBT){
	uint16_t pba;
	uint8_t bb_marker;

	for ( pba=0; pba<1024; pba++){
		while( W25N_page_data_read( pba*64 ) == W25N_ERR_BUSY);

		while( W25N_read_data( 3, &bb_marker, 1) == W25N_ERR_BUSY);

		if ( bb_marker != 0xFF ){
			W25N_SET_BIT( BBBT[pba/32], pba%32);
		} else {
			W25N_RESET_BIT( BBBT[pba/32], pba%32);
		}
	}

}

//LEGACY
#if 0
//initializes the interface towards the W25N memory (SPI,DMA)
void W25N_init(void){
	W25N_SPI_init();
	W25N_DMA_CLOCK_ENABLE();
	W25N_DMA_FirstConfig_rx();
	W25N_DMA_FirstConfig_tx();
	W25N_DMA_Enable();

}

//deinitializes the interface towards the W25N memory
void W25N_deinit(void){
	W25N_DMA_Disable();

	LL_DMA_DeInit( W25N_DMAn, W25N_LL_DMA_STREAM_TX);
	LL_DMA_DeInit( W25N_DMAn, W25N_LL_DMA_STREAM_RX);

	LL_SPI_DeInit( W25N_SPIn );

}
#endif //LEGACY
