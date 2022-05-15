#ifndef __ACAM_H_
#define __ACAM_H_

#include "main.h"

//#define IMG_BUFF_SIZE 	2048

#define ACAM_I2C_ADDR	0x78		// Device address

#define ACAM_I2C_READ		0		// Read status flag
#define ACAM_I2C_WRITE		1		// Write status flag
#define ACAM_I2C_WRITE_REG	2		//

#define I2C_ONGOING			0
#define I2C_COMPLETE		1

#define ACAM_CS_LOW()					( GPIOB->BSRR = (LL_GPIO_PIN_12 << 16) )
#define ACAM_CS_HIGH()					( GPIOB->BSRR = LL_GPIO_PIN_12 )

struct ACAM_I2C_Register{
	uint8_t HIGH;			// 16 MSBs of register address
	uint8_t LOW;		// 16 LSBs of register address
	uint8_t CMD;		// Command to be sent
} __PACKED;

struct I2C_Transaction{
	uint8_t type;			// Transaction type (read, write)
	uint32_t bytesLeft;		// Number of bytes to transfer, excl. i2c slave address
	uint8_t status;			// Communication status
	uint8_t retval;			// Returning value
};

extern volatile struct I2C_Transaction I2C_Trans;

extern struct ACAM_I2C_Register *acam_I2C;

//extern uint8_t buffer[IMG_BUFF_SIZE];


extern const struct ACAM_I2C_Register OV5642_5mp_RAW[];

extern const struct ACAM_I2C_Register OV5642_1920x1080_RAW[];

extern const struct ACAM_I2C_Register OV5642_1280x960_RAW[];

extern const struct ACAM_I2C_Register OV5642_640x480_RAW[];

extern const struct ACAM_I2C_Register OV5642_320x240[];

extern const struct ACAM_I2C_Register OV5642_QVGA_Preview[];

extern const struct ACAM_I2C_Register OV5642_JPEG_Capture_QSXGA[];

//Arducam general functions
uint8_t ACAM_TestComms(void);

//Arducam SPI functions
uint8_t ACAM_SPI_Read(uint8_t reg);

void ACAM_SPI_Write(uint8_t reg, uint8_t val);

void ACAM_spi_read_package(uint8_t * buff, uint16_t size);

/*
 * =====================
 * Arducam I2C functions
 * =====================
 */

/*
 * @brief Setup parameters for I2C communication (SADD, RD_WRN, NBYTES)
 */
void ACAM_I2C_Setup();

/*
 * @brief Read from data register
 */
uint8_t ACAM_I2C_Read(uint16_t reg);

void ACAM_I2C_Write(uint16_t reg, uint8_t command);

void ACAM_I2C_WriteSeq(const struct ACAM_I2C_Register commandSeq[]);


//Arducam general functions
uint8_t ACAM_TestComms(void);

void ACAM_select_JPEG(void);

void ACAM_select_RAW(uint8_t resolution);

void ACAM_start_capture(void);

uint32_t ACAM_get_image_size();

void ACAM_set_exposure(uint16_t nr_lines, uint8_t nr_lines_frac);

void ACAM_Reset(void);

__STATIC_INLINE void ACAM_exp_gain_manual(void){
	ACAM_I2C_Write(0x3503, 0x07);
}

__STATIC_INLINE void ACAM_exp_gain_auto(void){
	ACAM_I2C_Write(0x3503, 0x00);
}

__STATIC_INLINE uint8_t ACAM_is_cap_complete(void){
	return ( ACAM_SPI_Read(0x41) & 0x08 );
}

void ACAM_set_gain(uint8_t gain);

#endif //__ACAM_H_
