#ifndef _AD7147_H_
#define _AD7147_H_

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

// Register map
//=============
#define	PWR_CONTROL					0x00	// RW	Power & conversion control

#define	STAGE_CAL_EN				0x01	// RW	Ambient compensation control register 0
#define	AMB_COMP_CTRL0				0x02	// RW	Ambient compensation control register 1
#define	AMB_COMP_CTRL1				0x03	// RW	Ambient compensation control register 2
#define	AMB_COMP_CTRL2				0x04	// RW	Ambient compensation control register 3

#define	STAGE_LOW_INT_EN			0x05	// RW	Interrupt enable register 0
#define	STAGE_HIGH_INT_EN			0x06	// RW	Interrupt enable register 1
#define	STAGE_COMPLETE_INT_EN		0x07	// RW	Interrupt enable register 2
#define	STAGE_LOW_LIMIT_INT			0x08	// R	Low limit interrupt status register 0
#define	STAGE_HIGH_LIMIT_INT		0x09	// R	High limit interrupt status register 1
#define	STAGE_COMPLETE_LIMIT_INT	0x0A	// R	Interrupt status register 2

#define	ADCRESULT_S0				0x0B	// R	ADC stage 0 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S1				0x0C	// R	ADC stage 1 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S2				0x0D	// R	ADC stage 2 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S3				0x0E	// R	ADC stage 3 result (uncompensated) actually located in SRAM

#define	ADCRESULT_S4				0x0F	// R	ADC stage 4 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S5				0x10	// R	ADC stage 5 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S6				0x11	// R	ADC stage 6 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S7				0x12	// R	ADC stage 7 result (uncompensated) actually located in SRAM

#define	ADCRESULT_S8				0x13	// R	ADC stage 8 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S9				0x14	// R	ADC stage 9 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S10				0x15	// R	ADC stage 10 result (uncompensated) actually located in SRAM
#define	ADCRESULT_S11				0x16	// R	ADC stage 11 result (uncompensated) actually located in SRAM

#define	DEVID						0x17	// R	I.D. Register

#define	THRES_STAT_REG0				0x40	// R	Current threshold status register 0
#define	THRES_STAT_REG1				0x41	// R	Current threshold status register 1
#define	PROX_STAT_REG				0x42	// R	Current proximity status register 2


// Ram map - these registers are defined as we go along
//=====================================================
#define STAGE0_CONNECTION	0x80
#define STAGE1_CONNECTION	0x88
#define STAGE2_CONNECTION	0x90
#define STAGE3_CONNECTION	0x98
#define STAGE4_CONNECTION	0xA0
#define STAGE5_CONNECTION	0xA8
#define STAGE6_CONNECTION	0xB0
#define STAGE7_CONNECTION	0xB8
#define STAGE8_CONNECTION	0xC0
#define STAGE9_CONNECTION	0xC8
#define STAGE10_CONNECTION	0xD0
#define STAGE11_CONNECTION	0xD8


#define STAGE0				0xE0
#define STAGE0_AMBIENT		0xF1
#define STAGE0_MAX_AVG		0xF9
#define STAGE0_UPP_THRES	0xFA
#define STAGE0_MIN_AVG		0x100
#define STAGE0_LWR_THRES	0x101

#define STAGE1				0x104
#define STAGE1_AMBIENT		0x115
#define STAGE1_MAX_AVG		0x11D
#define STAGE1_UPP_THRES	0x11E
#define STAGE1_MIN_AVG		0x124
#define STAGE1_LWR_THRES	0x125

#define STAGE2				0x128
#define STAGE2_AMBIENT		0x139
#define STAGE2_MAX_AVG		0x141
#define STAGE2_UPP_THRES	0x142
#define STAGE2_MIN_AVG		0x148
#define STAGE2_LWR_THRES	0x149

#define STAGE3				0x14C
#define STAGE3_AMBIENT		0x15D
#define STAGE3_MAX_AVG		0x165
#define STAGE3_UPP_THRES	0x166
#define STAGE3_MIN_AVG		0x16C
#define STAGE3_LWR_THRES	0x16D

#define STAGE4				0x170
#define STAGE4_AMBIENT		0x181
#define STAGE4_MAX_AVG		0x189
#define STAGE4_UPP_THRES	0x18A
#define STAGE4_MIN_AVG		0x190
#define STAGE4_LWR_THRES	0x191

#define STAGE5				0x194
#define STAGE5_AMBIENT		0x1A5
#define STAGE5_MAX_AVG		0x1AD
#define STAGE5_UPP_THRES	0x1AE
#define STAGE5_MIN_AVG		0x1B4
#define STAGE5_LWR_THRES	0x1B5

#define STAGE6				0x1B8
#define STAGE6_AMBIENT		0x1C9
#define STAGE6_MAX_AVG		0x1D1
#define STAGE6_UPP_THRES	0x1D2
#define STAGE6_MIN_AVG		0x1D8
#define STAGE6_LWR_THRES	0x1D9

#define STAGE7				0x1DC
#define STAGE7_AMBIENT		0x1ED
#define STAGE7_MAX_AVG		0x1F5
#define STAGE7_UPP_THRES	0x1F6
#define STAGE7_MIN_AVG		0x1FC
#define STAGE7_LWR_THRES	0x1FD

#define STAGE8				0x200
#define STAGE8_AMBIENT		0x211
#define STAGE8_MAX_AVG		0x219
#define STAGE8_UPP_THRES	0x21A
#define STAGE8_MIN_AVG		0x220
#define STAGE8_LWR_THRES	0x221

#define STAGE9				0x224
#define STAGE9_AMBIENT		0x234
#define STAGE9_MAX_AVG		0x23D
#define STAGE9_UPP_THRES	0x23E
#define STAGE9_MIN_AVG		0x244
#define STAGE9_LWR_THRES	0x245

#define STAGE10				0x248
#define STAGE10_AMBIENT		0x259
#define STAGE10_MAX_AVG		0x261
#define STAGE10_UPP_THRES	0x262
#define STAGE10_MIN_AVG		0x268
#define STAGE10_LWR_THRES	0x269

#define STAGE11				0x26C
#define STAGE11_AMBIENT		0x27D
#define STAGE11_MAX_AVG		0x285
#define STAGE11_UPP_THRES	0x286
#define STAGE11_MIN_AVG		0x28C
#define STAGE11_LWR_THRES	0x28D

#define NB_OF_INT			3
#define POWER_UP_INTERRUPT	0x03FF
//For the memory map of the ADuC841 registers
#define STAGE_START_ADDRESS		10
#define NB_OF_REGS_PER_STAGE	36

#define NUMBER_OF_SENSORS		12
#define NB_OF_CONN_REGS			8

#define NUMBER_OF_AD7147_REGISTERS				23	//[0...23 inc]=1st set of registers [0...23 inc]

#define POWER_MODE_FULL				0b00
#define POWER_MODE_SHUTDOWN			0b01	
#define POWER_MODE_LOW				0b10

#define DECIMATION_RATE_256			0b00
#define DECIMATION_RATE_128			0b01
#define DECIMATION_RATE_64			0b10

#define DELAY_200					0b00
#define DELAY_400					0b01
#define DELAY_600					0b10
#define DELAY_800					0b11

#define CS_NC						0b00
#define CS_NEG						0b01
#define CS_POS						0b10
#define	CS_UNUSED					0b11

#define CS_SINGLE_ENDED_POS(num)	(uint16_t)(0xFFFF & ~(0b11 << (num << 1)) | (CS_POS << (num << 1)))
#define CS_SINGLE_ENDED_NEG(num)	(uint16_t)(0xFFFF & ~(0b11 << (num << 1)) | (CS_NEG << (num << 1)))

#define CS_SETUP_SINGLE_POS(reg)		(uint16_t)(reg & ~(0b1111 << 12) | (0b0001 << 12))
#define CS_SETUP_SINGLE_NEG(reg)		(uint16_t)(reg & ~(0b1111 << 12) | (0b0010 << 12))
#define CS_SETUP_DIFF(reg)				(uint16_t)(reg & ~(0b1111 << 12) | (0b0011 << 12))

class AD7147{
public:
	
	volatile uint16_t  SensorValues[NUMBER_OF_SENSORS];
	volatile uint16_t  ButtonStatus;

    AD7147();
    bool begin(int CS_PIN, int INT_PIN);
    
    bool update(void);
    
	//************* TODO: these ***********//
	void setStageThresholds(const byte stage, const uint16_t threshLow, const uint16_t threshHigh);
	void setPowerMode(const byte mode);
	void setConversionDelay(const byte delay);
	void setDecimationRate(const byte rate);
	//************** end TODO *************//
	
	void setConnection(byte stage, byte pos);
	
	void setIntTypeThreshold();
	void setIntTypeConversion();
	
	void shutdown();
	
    uint16_t getDeviceID();
    
    void  write(const uint16_t RegisterAddress, const byte NumberOfRegisters, volatile uint16_t *DataBuffer, const byte OffsetInBuffer);
    void  read(const uint16_t RegisterStartAddress, const byte NumberOfRegisters, volatile uint16_t *DataBuffer, const uint16_t OffsetInBuffer);

    void init(uint8_t a);
    
private:
    volatile uint16_t AD7147Registers[NUMBER_OF_AD7147_REGISTERS];
    int CS_PIN, INT_PIN;
    
    void forceCalibration(void);
	uint16_t DecodeButtons(const volatile uint16_t HighLimitStatusRegister);
};

#endif