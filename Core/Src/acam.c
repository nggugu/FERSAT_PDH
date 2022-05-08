#include "acam.h"

//uint8_t buffer[IMG_BUFF_SIZE];

volatile struct I2C_Transaction I2C_Trans;
struct ACAM_I2C_Register *acam_I2C;

void ACAM_Reset(void){
	ACAM_SPI_Write(0x06,0x05);		//reset sensor via SPI
	wait_for(100,TIM_UNIT_MS);

	ACAM_SPI_Write(0x07,0x80);		//reset CPLD
	wait_for(100,TIM_UNIT_MS);
	ACAM_SPI_Write(0x07,0x00);
	wait_for(100,TIM_UNIT_MS);

	return;
}

//test I2C and SPI interface
//if ok returns 1, else 0
uint8_t ACAM_TestComms(void){
	if( ACAM_I2C_Read(0x300a)!=0x56 ) return 0;
	if( ACAM_SPI_Read(0x40)!=0x73 ) return 0;

	return 1;
}

void ACAM_select_JPEG(void){
	uint8_t readout;
	ACAM_I2C_Write(0x3008, 0x08);	//reset sensor via I2C
	wait_for(100,TIM_UNIT_MS);

	ACAM_I2C_WriteSeq(OV5642_QVGA_Preview);
	ACAM_I2C_WriteSeq(OV5642_JPEG_Capture_QSXGA);
	//ACAM_i2c_write_seq(OV5642_320x240);
	//ACAM_i2c_write_seq(OV5642_JPEG_Capture_QSXGA);
	wait_for(10,TIM_UNIT_MS);

	ACAM_I2C_Write(0x3818, 0xa8);
	ACAM_I2C_Write(0x3621, 0x10);
	ACAM_I2C_Write(0x3801, 0xb0);
	ACAM_I2C_Write(0x4407, 0x04);

	readout = ACAM_SPI_Read(0x03);
	ACAM_SPI_Write(0x03, readout|0x02);

	ACAM_I2C_Write(0x5001, 0x7f);

	return;
}

//resolution selection
//	1	- 2592x1944 (5mp)
//	2	- 1920x1080
//	3	- 1280x960
//	4	- 640x480
void ACAM_select_RAW(uint8_t resolution){
	uint8_t readout;

	ACAM_I2C_Write(0x3008, 0x08);	//reset sensor via I2C
	wait_for(100,TIM_UNIT_MS);

	ACAM_I2C_WriteSeq(OV5642_1280x960_RAW);

	switch(resolution)
	{
		case 1:
		{
			ACAM_I2C_WriteSeq(OV5642_5mp_RAW);
			break;
		}
		case 2:
		{
			ACAM_I2C_WriteSeq(OV5642_5mp_RAW);
			ACAM_I2C_WriteSeq(OV5642_1920x1080_RAW);
			break;
		}
		case 3:
		{
			break;
		}
		case 4:
		{
			ACAM_I2C_WriteSeq(OV5642_640x480_RAW);
			break;
		}
		default:
		{
			ACAM_I2C_WriteSeq(OV5642_5mp_RAW);
			break;
		}
	}
	ACAM_I2C_Write(0x5001,0x00);  //disable auto white balance

	wait_for(10,TIM_UNIT_MS);


	readout = ACAM_SPI_Read(0x03);
	ACAM_SPI_Write(0x03, readout|0x02);

	return;
}

void ACAM_start_capture(void){
	wait_for(500,TIM_UNIT_MS);	//wait for manual exposure settings to steady

	ACAM_SPI_Write(0x04, 0x01); 	//clear fifo done
	ACAM_SPI_Write(0x04, 0x01);

	ACAM_SPI_Write(0x04, 0x30);	//fifo pointers defaulted

	ACAM_SPI_Write(0x01, 0x00);	//capture one frame

	wait_for(1,TIM_UNIT_MS);

	ACAM_SPI_Write(0x04, 0x02); //start capture

	return;
}

uint32_t ACAM_get_image_size(){
	uint32_t s1,s2,s3;

	s1 = 0x000000FF & (uint32_t)ACAM_SPI_Read(0x42);
	s2 = 0x000000FF & (uint32_t)ACAM_SPI_Read(0x43);
	s3 = 0x0000007F & (uint32_t)ACAM_SPI_Read(0x44);

	s3 = (s3<<16)+(s2<<8)+s1;
	return s3;
}

void ACAM_set_exposure(uint16_t nr_lines, uint8_t nr_lines_frac){
	uint16_t max_lines;

	max_lines = ((uint16_t)ACAM_I2C_Read(0x350C)<<8) | ((uint16_t)ACAM_I2C_Read(0x350D));

	if( nr_lines > max_lines ){
		ACAM_I2C_Write( 0x350C, (uint8_t)(nr_lines>>8) );
		ACAM_I2C_Write( 0x350D, (uint8_t)nr_lines );
	}

	if( nr_lines==0 ) return;	//nr_lines must be >=1

	ACAM_I2C_Write( 0x3500, (uint8_t)((nr_lines)>>12 & 0x00FF) );
	ACAM_I2C_Write( 0x3501, (uint8_t)(nr_lines>>4) );
	ACAM_I2C_Write( 0x3502, ((uint8_t)(nr_lines<<4)) | (nr_lines_frac & 0x0F) );

	return;

}

//valid gain values are 1-15; 16, 18, 20 ,... ,32
//if set to e.g. 19, actual gain will be 18
void ACAM_set_gain(uint8_t gain){
	if( gain>0 && gain<=16 ){
		ACAM_I2C_Write( 0x350B, (uint8_t)((gain-1)<<4) );

	} else if( gain>16 && gain<=32 ){
		ACAM_I2C_Write( 0x350B, (uint8_t)((gain/2-1)<<4) | 0x0F );

	}
	return;
}

uint8_t ACAM_SPI_Read(uint8_t reg){
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

void ACAM_SPI_Write(uint8_t reg, uint8_t val){
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
/*
void ACAM_spi_read_package(uint8_t * buff, uint16_t size){
	uint8_t command[2] = { 0x3C, 0x00 };
	uint8_t dummy;
	uint8_t i;

	ACAM_CS_LOW();
	wait_for(1,TIM_UNIT_US);

	for( i=0; i<2; i++){
		ACAM_DMA_ConfigEnable_tx();
		if (i==0) ACAM_DMA_config_tx( 1, &command[0] );
		else ACAM_DMA_config_tx( size, &command[1] );

		ACAM_DMA_ConfigEnable_rx();
		if (i==0) ACAM_DMA_config_rx( 1, &dummy );
		else ACAM_DMA_config_rx( size, buff );

		ACAM_DMA_start_rx();
		ACAM_DMA_start_tx();

		while(!DMA_trans_complete_tx);
		DMA_trans_complete_tx = 0;

		while(!DMA_trans_complete_rx);
		DMA_trans_complete_rx = 0;

		while( !LL_SPI_IsActiveFlag_TXE(ACAM_SPIn) );
		while( LL_SPI_IsActiveFlag_BSY(ACAM_SPIn) );
	}

	wait_for(1,TIM_UNIT_US);
	ACAM_CS_HIGH();
	wait_for(10,TIM_UNIT_US);

	return;

}
*/
/*
 * @brief Setup parameters for I2C communication (SADD, RD_WRN, NBYTES)
 */
void ACAM_I2C_Setup() {
	LL_I2C_SetTransferSize(I2C1, I2C_Trans.bytesLeft);	// Set bytes to be written/received

	// Check transfer type and set CR2 bit accordingly
	if ((I2C_Trans.type == ACAM_I2C_WRITE) || (I2C_Trans.type == ACAM_I2C_WRITE_REG)) {
		CLEAR_BIT(I2C1->CR2, I2C_CR2_RD_WRN);
	} else if (I2C_Trans.type == ACAM_I2C_READ) {
		SET_BIT(I2C1->CR2, I2C_CR2_RD_WRN);
	}

	// Set slave address
	LL_I2C_SetSlaveAddr(I2C1, ACAM_I2C_ADDR);
}

/*
 * @brief Read from data register
 */
uint8_t ACAM_I2C_Read(uint16_t reg){
	struct ACAM_I2C_Register currentReg;

	acam_I2C = &currentReg;

	// Splitting register value into low and high parts
	acam_I2C->HIGH = (uint8_t)( (reg >> 8) & 0x00FF );
	acam_I2C->LOW = (uint8_t)( reg & 0x00FF );

	// Send addresses of registers we want to read from and the type of communication we want
	I2C_Trans.type = ACAM_I2C_WRITE;
	I2C_Trans.bytesLeft = 2;
	I2C_Trans.status = I2C_ONGOING;

	ACAM_I2C_Setup();

	// Wait for ongoing communication to stop
	while(LL_I2C_IsActiveFlag_BUSY(I2C1));

	// Enable Transfer Complete and Transfer Register Empty interrupts
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);	// Start the communication

	// Wait for communication to stop
	while(I2C_Trans.status == I2C_ONGOING);
	while(LL_I2C_IsActiveFlag_BUSY(I2C1));

	// Send addresses of registers we want to read from and the type of communication we want
	I2C_Trans.type = ACAM_I2C_READ;
	I2C_Trans.bytesLeft = 1;
	I2C_Trans.status = I2C_ONGOING;

	ACAM_I2C_Setup();

	// Enable Transfer Complete and Receive Register Not Empty interrupts
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_RX(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);	// Start the communication

	// Wait for communication to stop
	while(I2C_Trans.status == I2C_ONGOING);
	while(LL_I2C_IsActiveFlag_BUSY(I2C1));

	return I2C_Trans.retval;	// Return read value
}

void ACAM_I2C_Write(uint16_t reg, uint8_t command){
	struct ACAM_I2C_Register current_reg;

	acam_I2C = &current_reg;

	acam_I2C->HIGH = (uint8_t)((reg >> 8) & 0x00FF);
	acam_I2C->LOW = (uint8_t)( reg & 0x00FF);
	acam_I2C->CMD = command;

	I2C_Trans.type = ACAM_I2C_WRITE_REG;
	I2C_Trans.bytesLeft = 3;
	I2C_Trans.status = I2C_ONGOING;

	ACAM_I2C_Setup();

	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );

	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
	while( I2C_Trans.status == I2C_ONGOING );

	wait_for(10,TIM_UNIT_US);
	return;
}

void ACAM_I2C_WriteSeq(const struct ACAM_I2C_Register commandSeq[]) {
	uint16_t currentReg=0;

	I2C_Trans.type = ACAM_I2C_WRITE_REG;

	acam_I2C = (struct ACAM_I2C_Register *)(&commandSeq[currentReg]);

	while(commandSeq[currentReg].HIGH!=0xFF){
		I2C_Trans.bytesLeft = 3;
		I2C_Trans.status = I2C_ONGOING;

		ACAM_I2C_Setup();

		while(LL_I2C_IsActiveFlag_BUSY(I2C1));

		LL_I2C_EnableIT_TC(I2C1);
		LL_I2C_EnableIT_TX(I2C1);

		LL_I2C_GenerateStartCondition(I2C1);
		while(I2C_Trans.status == I2C_ONGOING);

		currentReg++;
		acam_I2C++;
		wait_for(10,TIM_UNIT_US);
	};

	return;
}
