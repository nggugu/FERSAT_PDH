#ifndef __ACAM_H_
#define __ACAM_H_

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"

//#define IMG_BUFF_SIZE 	2048

#define ACAM_I2C_ADDR	0x78

#define ACAM_I2C_READ		0
#define ACAM_I2C_WRITE		1
#define ACAM_I2C_WRITE_REG	2

#define I2C_ONGOING			0
#define I2C_COMPLETE		1

#define ACAM_CS_LOW()					( GPIOB->BSRR = (LL_GPIO_PIN_12 << 16) )
#define ACAM_CS_HIGH()					( GPIOB->BSRR = LL_GPIO_PIN_12 )

struct acam_i2c_reg{
	uint8_t reg_hi;
	uint8_t reg_lo;
	uint8_t cmd;
} __PACKED;

struct i2c_transaction{
	uint8_t type;
	uint8_t bytes_left;		//data bytes, excl. i2c slave address
	uint8_t status;
	uint8_t retval;
};

extern volatile struct i2c_transaction i2c_trans;

extern struct acam_i2c_reg *acam_i2c;

//extern uint8_t buffer[IMG_BUFF_SIZE];


extern const struct acam_i2c_reg OV5642_5mp_RAW[];

extern const struct acam_i2c_reg OV5642_1920x1080_RAW[];

extern const struct acam_i2c_reg OV5642_1280x960_RAW[];

extern const struct acam_i2c_reg OV5642_640x480_RAW[];

extern const struct acam_i2c_reg OV5642_320x240[];

extern const struct acam_i2c_reg OV5642_QVGA_Preview[];

extern const struct acam_i2c_reg OV5642_JPEG_Capture_QSXGA[];


//Arducam SPI functions
uint8_t ACAM_spi_read(uint8_t reg);

void ACAM_spi_write(uint8_t reg, uint8_t val);


//Arducam I2C functions
uint8_t ACAM_i2c_read(uint16_t reg);

void ACAM_i2c_write(uint16_t reg, uint8_t command);

void ACAM_i2c_write_seq(const struct acam_i2c_reg command_seq[]);


//Arducam general functions
uint8_t ACAM_test_comms(void);

void ACAM_reset(void);

void ACAM_select_JPEG(void);

void ACAM_select_RAW(uint8_t resolution);

void ACAM_start_capture(void);

uint32_t ACAM_get_image_size();

__STATIC_INLINE void ACAM_exp_gain_manual(void){
	ACAM_i2c_write(0x3503, 0x07);
}

__STATIC_INLINE void ACAM_exp_gain_auto(void){
	ACAM_i2c_write(0x3503, 0x00);
}

__STATIC_INLINE uint8_t ACAM_is_cap_complete(void){
	return ( ACAM_spi_read(0x41) & 0x08 );
}

void ACAM_set_exposure(uint16_t nr_lines, uint8_t nr_lines_frac);

void ACAM_set_gain(uint8_t gain);

#endif //__ACAM_H_
