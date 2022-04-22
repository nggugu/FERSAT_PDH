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
	LL_I2C_EnableIT_STOP(I2C1);
	LL_I2C_EnableIT_TC(I2C1);
	LL_I2C_EnableIT_TX(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	//LL_I2C_EnableIT_EVT(I2C1);
	//LL_I2C_EnableIT_BUF(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
	while( i2c_trans.status == I2C_ONGOING );

	LL_I2C_GenerateStopCondition(I2C1);
	while( LL_I2C_IsActiveFlag_BUSY(I2C1) );


	i2c_trans.type = ACAM_I2C_READ;
	i2c_trans.bytes_left = 0xFF;
	i2c_trans.status = I2C_ONGOING;

	LL_I2C_DisableIT_ADDR(I2C1);
	LL_I2C_DisableIT_STOP(I2C1);
	LL_I2C_DisableIT_TC(I2C1);
	LL_I2C_DisableIT_TX(I2C1);
	LL_I2C_DisableIT_RX(I2C1);

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
