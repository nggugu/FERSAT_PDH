#include "acam.h"

//uint8_t buffer[IMG_BUFF_SIZE];

volatile struct i2c_transaction i2c_trans;
struct acam_i2c_reg *acam_i2c;

void ACAM_reset(void){
	ACAM_spi_write(0x06,0x05);		//reset sensor via SPI
	wait_for(100,TIM_UNIT_MS);

	ACAM_spi_write(0x07,0x80);		//reset CPLD
	wait_for(100,TIM_UNIT_MS);
	ACAM_spi_write(0x07,0x00);
	wait_for(100,TIM_UNIT_MS);

	return;
}

//test I2C and SPI interface
//if ok returns 1, else 0
uint8_t ACAM_test_comms(void){
	//if( ACAM_i2c_read(0x300a)!=0x56 ) return 0;
	if( ACAM_spi_read(0x40)!=0x73 ) return 0;

	return 1;
}

void ACAM_select_JPEG(void){
	uint8_t readout;
	ACAM_i2c_write(0x3008, 0x08);	//reset sensor via I2C
	wait_for(100,TIM_UNIT_MS);

	ACAM_i2c_write_seq(OV5642_QVGA_Preview);
	ACAM_i2c_write_seq(OV5642_JPEG_Capture_QSXGA);
	//ACAM_i2c_write_seq(OV5642_320x240);
	//ACAM_i2c_write_seq(OV5642_JPEG_Capture_QSXGA);
	wait_for(10,TIM_UNIT_MS);

	ACAM_i2c_write(0x3818, 0xa8);
	ACAM_i2c_write(0x3621, 0x10);
	ACAM_i2c_write(0x3801, 0xb0);
	ACAM_i2c_write(0x4407, 0x04);

	readout = ACAM_spi_read(0x03);
	ACAM_spi_write(0x03, readout|0x02);

	ACAM_i2c_write(0x5001, 0x7f);

	return;
}

//resolution selection
//	1	- 2592x1944 (5mp)
//	2	- 1920x1080
//	3	- 1280x960
//	4	- 640x480
void ACAM_select_RAW(uint8_t resolution){
	uint8_t readout;

	ACAM_i2c_write(0x3008, 0x08);	//reset sensor via I2C
	wait_for(100,TIM_UNIT_MS);

	ACAM_i2c_write_seq(OV5642_1280x960_RAW);

	switch(resolution)
	{
		case 1:
		{
			ACAM_i2c_write_seq(OV5642_5mp_RAW);
			break;
		}
		case 2:
		{
			ACAM_i2c_write_seq(OV5642_5mp_RAW);
			ACAM_i2c_write_seq(OV5642_1920x1080_RAW);
			break;
		}
		case 3:
		{
			break;
		}
		case 4:
		{
			ACAM_i2c_write_seq(OV5642_640x480_RAW);
			break;
		}
		default:
		{
			ACAM_i2c_write_seq(OV5642_5mp_RAW);
			break;
		}
	}
	ACAM_i2c_write(0x5001,0x00);  //disable auto white balance

	wait_for(10,TIM_UNIT_MS);


	readout = ACAM_spi_read(0x03);
	ACAM_spi_write(0x03, readout|0x02);

	return;
}

void ACAM_start_capture(void){
	wait_for(500,TIM_UNIT_MS);	//wait for manual exposure settings to steady

	ACAM_spi_write(0x04, 0x01); 	//clear fifo done
	ACAM_spi_write(0x04, 0x01);

	ACAM_spi_write(0x04, 0x30);	//fifo pointers defaulted

	ACAM_spi_write(0x01, 0x00);	//capture one frame

	wait_for(1,TIM_UNIT_MS);

	ACAM_spi_write(0x04, 0x02); //start capture

	return;
}



uint32_t ACAM_get_image_size(){
	uint32_t s1,s2,s3;

	s1 = 0x000000FF & (uint32_t)ACAM_spi_read(0x42);
	s2 = 0x000000FF & (uint32_t)ACAM_spi_read(0x43);
	s3 = 0x0000007F & (uint32_t)ACAM_spi_read(0x44);

	s3 = (s3<<16)+(s2<<8)+s1;
	return s3;
}

void ACAM_set_exposure(uint16_t nr_lines, uint8_t nr_lines_frac){
	uint16_t max_lines;

	max_lines = ((uint16_t)ACAM_i2c_read(0x350C)<<8) | ((uint16_t)ACAM_i2c_read(0x350D));

	if( nr_lines > max_lines ){
		ACAM_i2c_write( 0x350C, (uint8_t)(nr_lines>>8) );
		ACAM_i2c_write( 0x350D, (uint8_t)nr_lines );
	}

	if( nr_lines==0 ) return;	//nr_lines must be >=1

	ACAM_i2c_write( 0x3500, (uint8_t)((nr_lines)>>12 & 0x00FF) );
	ACAM_i2c_write( 0x3501, (uint8_t)(nr_lines>>4) );
	ACAM_i2c_write( 0x3502, ((uint8_t)(nr_lines<<4)) | (nr_lines_frac & 0x0F) );

	return;

}

//valid gain values are 1-15; 16, 18, 20 ,... ,32
//if set to e.g. 19, actual gain will be 18
void ACAM_set_gain(uint8_t gain){
	if( gain>0 && gain<=16 ){
		ACAM_i2c_write( 0x350B, (uint8_t)((gain-1)<<4) );

	} else if( gain>16 && gain<=32 ){
		ACAM_i2c_write( 0x350B, (uint8_t)((gain/2-1)<<4) | 0x0F );

	}
	return;
}

uint8_t ACAM_spi_read(uint8_t reg){
	uint8_t retval;

	//ACAM_CS_LOW();
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);
	wait_for(500,TIM_UNIT_MS);

	LL_SPI_TransmitData8( SPI2, reg);			//instruction transmit
	while( !LL_SPI_IsActiveFlag_RXNE(SPI2) );
	LL_SPI_ReceiveData8(SPI2);					//dummy read data B1

	while( !LL_SPI_IsActiveFlag_TXE(SPI2) );
	LL_SPI_TransmitData8( SPI2, 0x00);  			//dummy write data B2
	while( !LL_SPI_IsActiveFlag_RXNE(SPI2) );		//wait for final byte to arrive
	retval = LL_SPI_ReceiveData8(SPI2);		//read data

	while( !LL_SPI_IsActiveFlag_TXE(SPI2) );	//wait for TX empty - not necessary, but per protocol
	while( LL_SPI_IsActiveFlag_BSY(SPI2) );

	wait_for(500,TIM_UNIT_MS);
	//ACAM_CS_HIGH();
	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12);
	wait_for(500,TIM_UNIT_MS);

	return retval;
}

void ACAM_spi_write(uint8_t reg, uint8_t val){
	ACAM_CS_LOW();
	wait_for(1,TIM_UNIT_US);

	LL_SPI_TransmitData8( SPI2, (reg|0x80));			//instruction transmit
	while( !LL_SPI_IsActiveFlag_RXNE(SPI2) );
	LL_SPI_ReceiveData8(SPI2);					//dummy read data B1

	while( !LL_SPI_IsActiveFlag_TXE(SPI2) );
	LL_SPI_TransmitData8( SPI2, val);  			//dummy write data B2
	while( !LL_SPI_IsActiveFlag_RXNE(SPI2) );		//wait for final byte to arrive
	LL_SPI_ReceiveData8(SPI2);		//read data

	while( !LL_SPI_IsActiveFlag_TXE(SPI2) );	//wait for TX empty - not necessary, but per protocol
	while( LL_SPI_IsActiveFlag_BSY(SPI2) );

	wait_for(1,TIM_UNIT_US);
	ACAM_CS_HIGH();
	wait_for(10,TIM_UNIT_US);

	return;
}

uint8_t ACAM_i2c_read(uint16_t reg){
	struct acam_i2c_reg current_reg;

	acam_i2c = &current_reg;

	acam_i2c->reg_hi = (uint8_t)((reg >> 8) & 0x00FF);
	acam_i2c->reg_lo = (uint8_t)( reg & 0x00FF);

	i2c_trans.type = ACAM_I2C_WRITE;
	i2c_trans.bytes_left = 2;
	i2c_trans.status = I2C_ONGOING;

	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );

	LL_I2C_EnableIT_ADDR(I2C1);
	LL_I2C_EnableIT_NACK(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	LL_I2C_EnableIT_STOP(I2C1);
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);

	//LL_I2C_EnableIT_EVT(I2C1);
	//LL_I2C_EnableIT_BUF(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
	while( i2c_trans.status == I2C_ONGOING );

	LL_I2C_GenerateStopCondition(I2C1);
	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );


	i2c_trans.type = ACAM_I2C_READ;
	i2c_trans.bytes_left = 0xFF;
	i2c_trans.status = I2C_ONGOING;

	LL_I2C_EnableIT_ADDR(I2C1);
	LL_I2C_EnableIT_NACK(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	LL_I2C_EnableIT_STOP(I2C1);
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);

	//LL_I2C_EnableIT_EVT(I2C1);
	//LL_I2C_EnableIT_BUF(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
	while( i2c_trans.status == I2C_ONGOING );
	LL_I2C_GenerateStopCondition(I2C1);

	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );
	LL_I2C_AcknowledgeNextData( I2C1, LL_I2C_ACK );

	return i2c_trans.retval;

}

void ACAM_i2c_write(uint16_t reg, uint8_t command){
	struct acam_i2c_reg current_reg;

	acam_i2c = &current_reg;

	acam_i2c->reg_hi = (uint8_t)((reg >> 8) & 0x00FF);
	acam_i2c->reg_lo = (uint8_t)( reg & 0x00FF);
	acam_i2c->cmd = command;

	i2c_trans.type = ACAM_I2C_WRITE_REG;
	i2c_trans.bytes_left = 3;
	i2c_trans.status = I2C_ONGOING;

	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );

	LL_I2C_EnableIT_ADDR(I2C1);
	LL_I2C_EnableIT_NACK(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	LL_I2C_EnableIT_STOP(I2C1);
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);

	//LL_I2C_EnableIT_EVT(I2C1);
	//LL_I2C_EnableIT_BUF(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
	while( i2c_trans.status == I2C_ONGOING );
	LL_I2C_GenerateStopCondition(I2C1);

	wait_for(10,TIM_UNIT_US);
	return;
}

void ACAM_i2c_write_seq(const struct acam_i2c_reg command_seq[]){
	uint16_t current_reg=0;

	i2c_trans.type = ACAM_I2C_WRITE_REG;

	acam_i2c = (struct acam_i2c_reg *)(&command_seq[current_reg]);

	while( command_seq[current_reg].reg_hi!=0xFF ){
		i2c_trans.bytes_left = 3;
		i2c_trans.status = I2C_ONGOING;

		while( LL_I2C_IsActiveFlag_BUSY(I2C1) );

		LL_I2C_EnableIT_ADDR(I2C1);
		LL_I2C_EnableIT_NACK(I2C1);
		LL_I2C_EnableIT_RX(I2C1);
		LL_I2C_EnableIT_STOP(I2C1);
		LL_I2C_EnableIT_TC(I2C1);
		LL_I2C_EnableIT_TX(I2C1);

		//LL_I2C_EnableIT_EVT(I2C1);
		//LL_I2C_EnableIT_BUF(I2C1);

		LL_I2C_GenerateStartCondition(I2C1);
		while( i2c_trans.status == I2C_ONGOING );
		LL_I2C_GenerateStopCondition(I2C1);

		current_reg++;
		acam_i2c++;
		wait_for(10,TIM_UNIT_US);
	};

	return;

}

