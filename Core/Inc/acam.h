#ifndef __ACAM_H_
#define __ACAM_H_

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"

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

/*
 * =====================
 * Arducam I2C functions
 * =====================
 */

/*
 * @brief Setup parameters for I2C communication (SADD, RD_WRN, NBYTES)
 */
void ACAM_I2C_Setup();

uint8_t ACAM_I2C_Read(uint16_t reg);

void ACAM_I2C_Write(uint16_t reg, uint8_t command);

void ACAM_I2C_WriteSeq(const struct ACAM_I2C_Register command_seq[]);


//Arducam general functions
uint8_t ACAM_TestComms(void);

void ACAM_Reset(void);

#endif //__ACAM_H_
