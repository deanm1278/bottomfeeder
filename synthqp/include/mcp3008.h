#ifndef LIB_MCP3008_H
#define LIB_MCP3008_H

#include <SPI.h>

#define MCP3008_NUM_CHANNELS 8
#define MCP3008_SINGLE_ENDED 0xC000
#define MCP3008_DIFFERENTIAL 0x8000

class mcp3008 {
public:
	mcp3008(int CS_PIN, SPISettings settings = SPISettings(1350000, MSBFIRST, SPI_MODE0)) : _cs_pin(CS_PIN),
	_spi_settings(settings) {}

	~mcp3008() {}

	void begin()
	{
		//_spi_init();

		pinMode(_cs_pin, OUTPUT);
		digitalWrite(_cs_pin, HIGH);
	}

	uint16_t readChannelSingle(uint8_t ch)
	{
		uint16_t high, ret;
		uint8_t low;
		uint16_t toWrite = MCP3008_SINGLE_ENDED | ((uint16_t)ch << 11);
		
		_beginTransaction();
		high = _transfer16(toWrite);
		low = _transfer8();
		_endTransaction();
		
		uint16_t hbits = (high << 1);
		hbits = hbits & 0x3FE;
		
		uint16_t lbits = ((uint16_t)low >> 7);
		lbits = lbits & 0x01;
		
		ret = hbits | lbits;
		return ret;
	}

	void readAllChannels(uint16_t *buf, uint8_t num = MCP3008_NUM_CHANNELS)
	{
		for(int i=0; i<num; i++){
			buf[i] = readChannelSingle(i);
		}
	}
private:
	uint8_t _cs_pin;

	SPISettings _spi_settings;

	uint16_t _beginTransaction()
	{
		SPI.beginTransaction(_spi_settings);
		digitalWrite(_cs_pin, LOW);
	}

	void _endTransaction()
	{ 
		digitalWrite(_cs_pin, HIGH);
		SPI.endTransaction();
	}

	uint16_t _transfer16(uint16_t val = 0x0000)
	{
		return SPI.transfer16(val);
	}

	uint16_t _transfer8(uint8_t val = 0x00)
	{
		return SPI.transfer(val);
	}

	void _spi_init() 
	{
		SPI.begin();
	}
};

#endif