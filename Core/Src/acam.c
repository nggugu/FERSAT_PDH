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

	ACAM_I2C_Setup(I2C1, ACAM_I2C_ADDR);

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

	ACAM_I2C_Setup(I2C1, ACAM_I2C_ADDR);

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
	while( I2C_Trans.status == I2C_ONGOING );
	LL_I2C_GenerateStopCondition(I2C1);

	wait_for(10,TIM_UNIT_US);
	return;
}
