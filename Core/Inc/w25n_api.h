//NOTE: pull-up resisor (10k will do) MUST be connected between /CS
//and VCC for MIKROE Flash 5 Click board (physically, not GPIO)

#ifndef __W25N_API_H
#define __W25N_API_H

#include "main.h"

typedef enum{
	W25N_SR1 = 1,
	W25N_SR2 = 2,
	W25N_SR3 = 3

} W25N_SRn;

#define W25N_SR1_BP_MASK			0b01111100
#define W25N_SR1_SRP_MASK			0b10000001
#define W25N_SR1_WPE_MASK			0b00000010
#define W25N_SR2_ECC_MASK			0b00010000
#define W25N_SR2_BUF_MASK 			0b00001000
#define W25N_SR2_OTP_L_MASK			0b10000000
#define W25N_SR2_OTP_E_MASK			0b01000000
#define W25N_SR3_LUTF_MASK 			0b01000000
#define W25N_SR3_ECC_ERR_MASK		0b00110000
#define W25N_SR3_PFAIL_MASK			0b00001000
#define W25N_SR3_EFAIL_MASK			0b00000100
#define W25N_SR3_WEL_MASK 			0b00000010
#define W25N_SR3_BUSY_MASK			0b00000001

#define W25N_ERR_BUSY				0
#define W25N_INSTR_ISSUED			1

//returns HI byte for a given uint16_t
#define ISOLATE_HI_BYTE(B)		((uint8_t)( ( (B) >> 0x08 ) & 0x00FF))
//returns LO byte for a given uint16_t
#define ISOLATE_LO_BYTE(B)		((uint8_t)( (B) & 0x00FF ))

//returns 16 bit page address for given block number and page number within block
#define W25N_BNPN_TO_PA(B,P)	((uint16_t)( (((uint16_t)(B)) << 0x06) ^ (((uint16_t)(P)) & 0x003Fu) ))

//word has to be 32 bit
#define W25N_SET_BIT(WORD,BIT)			( (WORD) |= (0x00000001 << (BIT)) )
#define W25N_RESET_BIT(WORD,BIT)		( (WORD) &= ~(0x00000001 << (BIT)) )

extern const uint8_t W25N_sr_address[3];

uint8_t W25N_init(void);
void W25N_deinit(void);

uint8_t W25N_check_status(W25N_SRn srn, uint8_t mask);

uint8_t W25N_read_sr(W25N_SRn srn);

void W25N_write_sr(W25N_SRn srn, uint8_t srval);

__STATIC_INLINE uint8_t W25N_block_protect_status(void){
	return W25N_check_status(W25N_SR1, (uint8_t)W25N_SR1_BP_MASK);
}

__STATIC_INLINE uint8_t W25N_check_OTP_locked(void){
	return W25N_check_status(W25N_SR2, (uint8_t)W25N_SR2_OTP_L_MASK);
}

__STATIC_INLINE uint8_t W25N_check_OTP_mode(void){
	return W25N_check_status(W25N_SR2, (uint8_t)W25N_SR2_OTP_E_MASK);
}

__STATIC_INLINE uint8_t W25N_check_ECC_on(void){
	return W25N_check_status(W25N_SR2, (uint8_t)W25N_SR2_ECC_MASK);
}

__STATIC_INLINE uint8_t W25N_check_BUF_mode(void){
	return W25N_check_status(W25N_SR2, (uint8_t)W25N_SR2_BUF_MASK);
}

__STATIC_INLINE uint8_t W25N_check_busy(void){
	return W25N_check_status(W25N_SR3, (uint8_t)W25N_SR3_BUSY_MASK);
}

__STATIC_INLINE uint8_t W25N_check_prog_fail(void){
	return W25N_check_status(W25N_SR3, (uint8_t)W25N_SR3_PFAIL_MASK);
}

__STATIC_INLINE uint8_t W25N_check_erase_fail(void){
	return W25N_check_status(W25N_SR3, (uint8_t)W25N_SR3_EFAIL_MASK);
}

__STATIC_INLINE uint8_t W25N_check_write_enabled(void){
	return W25N_check_status(W25N_SR3, (uint8_t)W25N_SR3_WEL_MASK);
}

__STATIC_INLINE uint8_t W25N_check_LUT_full(void){
	return W25N_check_status(W25N_SR3, (uint8_t)W25N_SR3_LUTF_MASK);
}

//if there was any ECC action, returns the values according to
//W25N01GV user manual section 7.3.2
//ret. val: 0x00
//			0x01
//			0x02
//			0x03
__STATIC_INLINE uint8_t W25N_ECC_err_status(void){
	return ( (( W25N_read_sr(W25N_SR3) & ((uint8_t)W25N_SR3_ECC_ERR_MASK) ) >> 0x04) & 0x03 );
}

void W25N_disable_block_protect(void);

void W25N_enter_OTP_mode(void);

void W25N_exit_OTP_mode(void);

void W25N_set_OTP_lock(void);

void W25N_enter_BUF_mode(void);

void W25N_exit_BUF_mode(void);

uint8_t W25N_reset(void);

uint32_t W25N_jedec_id(void);


__STATIC_INLINE void W25N_write_enable(void){
	W25N_instruction_execute( w25n_write_enable, NULL, NULL, 0);
}

__STATIC_INLINE void W25N_write_disable(void){
	W25N_instruction_execute( w25n_write_disable, NULL, NULL, 0);
}

void W25N_bb_manage(uint16_t lba, uint16_t pba);

void W25N_read_bbm_lut(uint8_t * container);

uint8_t W25N_block_erase(uint16_t block_nr);

uint8_t W25N_prog_data_load(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes);

uint8_t W25N_rand_prog_data_load(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes);

uint8_t W25N_prog_execute(uint16_t page_addr);

uint8_t W25N_page_data_read(uint16_t page_addr);

uint8_t W25N_read_data(uint16_t col_addr, uint8_t * data, uint16_t nr_bytes);

void W25N_scan_factory_BB(uint32_t * BBBT);

#endif //__W25N_API_H
