#ifndef LIB_FPGA_H_
#define LIB_FPGA_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "Arduino.h"
#include "SPI.h"

#include "buffer.h"
#include "hsm_id.h"

#define CRESET_B        A3
#define CDONE           A4

#define FPGA_CS			10

#define FPGA_CLKFREQ 150000000
#define FPGA_N 1024

#define DATA_LENGTH (4096)

#define FPGA_REG_MASK(reg) (reg & 0xFF) << 6

#define FPGA_W0 0x00
#define FPGA_W1 0x01
#define FPGA_W2 0x02

#define FPGA_W0_FREQ 0x03
#define FPGA_W1_FREQ 0x04
#define FPGA_W2_FREQ 0x05

#define FPGA_PWM0 0x06 //cutoff
#define FPGA_PWM1 0x07 //reso
#define FPGA_PWM2 0x08 //noise
#define FPGA_PWM3 0x09 //vca
#define FPGA_PWM4 0x0A //sub

#define FPGA_VOL 0x0B
#define FPGA_ENABLE 0x0C

#define FPGA_A_INTERVAL_HIGH 0x0D
#define FPGA_A_INTERVAL_LOW 0x0E
#define FPGA_D_INTERVAL_HIGH 0x0F
#define FPGA_D_INTERVAL_LOW 0x10
#define FPGA_R_INTERVAL_HIGH 0x11
#define FPGA_R_INTERVAL_LOW 0x12
#define FPGA_SUS_LEVEL 0x13
#define FPGA_ENV0_PWM0_MUL 0x14
#define FPGA_ENV0_PWM2_MUL 0x15

#define FPGA_PORT		0x16

#define FPGA_KEY_PRESSED 0x17

#define FPGA_SUB_OFF 0x00
#define FPGA_SUB0 0x01
#define FPGA_SUB1 0x02
#define FPGA_SUB2 0x04

#define FPGA_RW_BIT(bit) (bit << 15)
#define FPGA_WRITE_TYPE_BIT(bit) (bit << 14)
#define FPGA_CHANNEL_BITS(channel) ((channel & 0x03) << 10)
#define FPGA_BANK_BITS(bank) ((bank & 0x03) << 8)
#define FPGA_ADDR_BITS(addr) (addr & 0xFF)

#define FPGA_VOL_MASK(num)	(0x7 << (uint16_t)(3 * (num - 1)))
#define FPGA_VOL_BITS(num, vol)	((vol & 0x7) << (uint16_t)(3 * (num - 1)))

using namespace QP;
using namespace FW;

class FPGA : public QActive {
	
public:
	FPGA();
	~FPGA();
	
	void Start(uint8_t prio);
	
	protected:
	static QState InitialPseudoState(FPGA * const me, QEvt const * const e);
	static QState Root(FPGA * const me, QEvt const * const e);
	static QState Stopped(FPGA * const me, QEvt const * const e);
	static QState Started(FPGA * const me, QEvt const * const e);
	static QState WritingWave(FPGA * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 32,
		DEFER_QUEUE_COUNT = 4
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	QEvt const *m_deferQueueStor[DEFER_QUEUE_COUNT];
	QEQueue m_deferQueue;
	uint8_t m_id;
	char const * m_name;
	
private:
	
	void writeWaveSample(uint16_t channel, uint16_t sample, uint16_t value);

	void writeReg(uint8_t reg, uint16_t value);
	uint16_t readReg(uint8_t reg);
	
	SPISettings settings;
	buffer SDBuffer;
	
	//for writing waves
	uint8_t writeChannel;
	uint16_t writeVolume;
	uint16_t writePos;
};

#endif