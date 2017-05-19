#include "AD7147.h"
#include <SPI.h>

AD7147::AD7147(){
	
}

bool AD7147::begin(int CS_PIN, int INT_PIN){
	this->CS_PIN = CS_PIN;
	this->INT_PIN = INT_PIN;
	pinMode(CS_PIN, OUTPUT);
	//pinMode(INT_PIN, INPUT_PULLUP);
	digitalWrite(CS_PIN, HIGH);
	
	SPI.begin();
	
	//make sure we can get device id
	if(getDeviceID() == 0){
		return false;
	}
	
	//create all connections by default
	for(byte i=0; i<NUMBER_OF_SENSORS; i++){
		setConnection(i, i);
	}
	
	 //--------------------------------------------------------------------------//
	 //-------------------------Bank 1 Registers---------------------------------//
	 //--------------------------------------------------------------------------//
	 //Initialisation of the first register bank but not the STAGE_CAL_EN
	 AD7147Registers[PWR_CONTROL]=0x00B2; //Register 0x00
	 this->write(PWR_CONTROL, 1, AD7147Registers, PWR_CONTROL);

	 //Read high and low limit interrupt status before to enable the interrupts
	 this->read(STAGE_LOW_LIMIT_INT, 2, AD7147Registers, STAGE_LOW_LIMIT_INT); //Registers 0x08 & 0x09
	 AD7147Registers[AMB_COMP_CTRL0]=0x3230;					//Register 0x02
	 AD7147Registers[AMB_COMP_CTRL1]=0x819;					//Register 0x03
	 AD7147Registers[AMB_COMP_CTRL2]=0x832;					//Register 0x04
	 AD7147Registers[STAGE_LOW_INT_EN]=POWER_UP_INTERRUPT;	//Register 0x05
	 AD7147Registers[STAGE_HIGH_INT_EN]=POWER_UP_INTERRUPT;	//Register 0x06
	 AD7147Registers[STAGE_COMPLETE_INT_EN]=0x00;			//Register 0x07
	 this->write(AMB_COMP_CTRL0, 6, AD7147Registers, AMB_COMP_CTRL0);

	 //Enable data path for all sequences
	 AD7147Registers[STAGE_CAL_EN]=0xFFF;					//Register 0x01
	 this->write(STAGE_CAL_EN, 1, AD7147Registers, STAGE_CAL_EN);

	 //Set "FORCE_CAL" bit
	 AD7147Registers[AMB_COMP_CTRL0] |= 0x4000;
	 this->write(AMB_COMP_CTRL0, 1, AD7147Registers, AMB_COMP_CTRL0);

	 //Read High and Low Limit Status registers to clear INT pin
	 this->read(STAGE_LOW_LIMIT_INT, 2, AD7147Registers, STAGE_LOW_LIMIT_INT); //Registers 0x08 & 0x09

	
	return true;
}

void AD7147::setIntTypeThreshold(){
	AD7147Registers[STAGE_LOW_INT_EN]= 0xFFFF & ~(1 << 15);//Register 0x05
	AD7147Registers[STAGE_HIGH_INT_EN]= 0xFFFF & ~(1 << 15);	//Register 0x06
	AD7147Registers[STAGE_COMPLETE_INT_EN]=0x0000;	//Register 0x07
	this->write(STAGE_LOW_INT_EN, 3, AD7147Registers, STAGE_LOW_INT_EN);
}

void AD7147::setIntTypeConversion(){
	AD7147Registers[STAGE_LOW_INT_EN]=0x0000;//Register 0x05
	AD7147Registers[STAGE_HIGH_INT_EN]=0x0000;	//Register 0x06
	AD7147Registers[STAGE_COMPLETE_INT_EN]= 0x0000 | 1 << 11;	//Register 0x07
	this->write(STAGE_LOW_INT_EN, 3, AD7147Registers, STAGE_LOW_INT_EN);
}

bool AD7147::update(void)
{
	//Read thresholds and proximity registers
	this->read(STAGE_LOW_LIMIT_INT, 2, AD7147Registers, STAGE_LOW_LIMIT_INT);

	//Recover from errors if needed
	if (((AD7147Registers[STAGE_LOW_LIMIT_INT] & POWER_UP_INTERRUPT) != 0x0000) &&
	((AD7147Registers[STAGE_HIGH_LIMIT_INT] & POWER_UP_INTERRUPT) == 0x0000))
	{
		forceCalibration();
		return false;
	}
	else
	ButtonStatus=DecodeButtons(AD7147Registers[STAGE_HIGH_LIMIT_INT]);
	
	return true;
}

uint16_t AD7147::DecodeButtons(const volatile uint16_t HighLimitStatusRegister)
{
	uint16_t ButtonStatusValue=0;
	
	if ((HighLimitStatusRegister & 0x0001) == 0x0001)
	ButtonStatusValue |= 0x0001;
	
	if ((HighLimitStatusRegister & 0x0002) == 0x0002)
	ButtonStatusValue |= 0x0002;
	
	if ((HighLimitStatusRegister & 0x0004) == 0x0004)
	ButtonStatusValue |= 0x0004;
	
	if ((HighLimitStatusRegister & 0x0008) == 0x0008)
	ButtonStatusValue |= 0x0008;

	if ((HighLimitStatusRegister & 0x0010) == 0x0010)
	ButtonStatusValue |= 0x0010;

	if ((HighLimitStatusRegister & 0x0020) == 0x0020)
	ButtonStatusValue |= 0x0020;

	if ((HighLimitStatusRegister & 0x0040) == 0x0040)
	ButtonStatusValue |= 0x0040;

	if ((HighLimitStatusRegister & 0x0080) == 0x0080)
	ButtonStatusValue |= 0x0080;

	if ((HighLimitStatusRegister & 0x0100) == 0x0100)
	ButtonStatusValue |= 0x0100;

	if ((HighLimitStatusRegister & 0x0200) == 0x0200)
	ButtonStatusValue |= 0x0200;

	if ((HighLimitStatusRegister & 0x0400) == 0x0400)
	ButtonStatusValue |= 0x0400;

	if ((HighLimitStatusRegister & 0x0800) == 0x0800)
	ButtonStatusValue |= 0x0800;
	
	return (ButtonStatusValue);
}

void AD7147::setConnection(byte stage, byte pos){
	//this only supports single ended positive currently
	uint16_t StageBuffer[8];
	
	if(pos > 6){
		StageBuffer[0]=0xFFFF;
		StageBuffer[1]=CS_SETUP_SINGLE_POS(CS_SINGLE_ENDED_POS(pos - 7));
	}
	else{
		StageBuffer[0]= CS_SINGLE_ENDED_POS(pos);
		StageBuffer[1] = CS_SETUP_SINGLE_POS(0xFFFF);
	}
	
	StageBuffer[2]=0x0000;
	StageBuffer[3]=0x6424;
	StageBuffer[4]=1600;
	StageBuffer[5]=1600;
	StageBuffer[6]=1600;
	StageBuffer[7]=1600;
	this->write(STAGE0_CONNECTION + (stage * 8), 8, StageBuffer, 0);
}

uint16_t AD7147::getDeviceID(){
	uint16_t id = 0x00;
	this->read(DEVID, 1, &id, 0);
	return id;
}

void AD7147::forceCalibration(void)
{
	this->read(AMB_COMP_CTRL0, 1, AD7147Registers, AMB_COMP_CTRL0);
	AD7147Registers[AMB_COMP_CTRL0] |= 0x4000;//Set "forced cal" bool
	this->write(AMB_COMP_CTRL0, 1, AD7147Registers, AMB_COMP_CTRL0);
}

void AD7147::shutdown(){
	AD7147Registers[PWR_CONTROL] = 0x1;
	this->write(PWR_CONTROL, 1, AD7147Registers, PWR_CONTROL);
}

void AD7147::write(const uint16_t RegisterAddress, const byte NumberOfRegisters, volatile uint16_t *DataBuffer, const byte OffsetInBuffer)
{
	uint16_t ControlValue;
	uint16_t ValueToWrite;
	uint16_t RegisterIndex;

	//Create the 16-bool header
	ControlValue = 0xE000 | (RegisterAddress & POWER_UP_INTERRUPT);
	
	digitalWrite(CS_PIN, LOW);
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
	
	SPI.transfer16(ControlValue);
	
	for (RegisterIndex=0; RegisterIndex<NumberOfRegisters; RegisterIndex++)
	{
		ValueToWrite= *(DataBuffer+RegisterIndex+OffsetInBuffer);
		SPI.transfer16(ValueToWrite);
	}
	
	SPI.endTransaction();
	digitalWrite(CS_PIN, HIGH);
}


void AD7147::read(const uint16_t RegisterStartAddress, const byte NumberOfRegisters, volatile uint16_t *DataBuffer, const uint16_t OffsetInBuffer)
{
	uint16_t ControlValue;
	byte RegisterIndex;

	//Create the 16-bool header
	ControlValue = 0xE400 | (RegisterStartAddress & 0x03FF);
	
	digitalWrite(CS_PIN, LOW);
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
	
	SPI.transfer16(ControlValue);
	
	for (RegisterIndex=0; RegisterIndex<NumberOfRegisters; RegisterIndex++)
	{
		*(DataBuffer+OffsetInBuffer+RegisterIndex) = SPI.transfer16(0x00);
	}
	
	SPI.endTransaction();
	digitalWrite(CS_PIN, HIGH);
}