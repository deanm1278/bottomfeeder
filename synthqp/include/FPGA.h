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

#define FPGA_W0_FREQ 0x00
#define FPGA_W1_FREQ 0x01
#define FPGA_W2_FREQ 0x02

#define FPGA_PWM0 0x03 //cutoff
#define FPGA_PWM1 0x04 //reso
#define FPGA_PWM2 0x05 //noise
#define FPGA_PWM3 0x06 //vca
#define FPGA_PWM4 0x07 //sub

#define FPGA_VOL 0x08
#define FPGA_ENABLE 0x09

#define FPGA_A_INTERVAL_HIGH 0x0a
#define FPGA_A_INTERVAL_LOW 0x0b
#define FPGA_D_INTERVAL_HIGH 0x0c
#define FPGA_D_INTERVAL_LOW 0x0d
#define FPGA_R_INTERVAL_HIGH 0x0e
#define FPGA_R_INTERVAL_LOW 0x0f
#define FPGA_SUS_LEVEL 0x10
#define FPGA_ENV0_PWM0_MUL 0x11
#define FPGA_ENV0_PWM2_MUL 0x12

#define FPGA_PORT		0x13

#define FPGA_KEY_PRESSED 0x14

#define FPGA_SUB_OFF 0x00
#define FPGA_SUB0 0x01
#define FPGA_SUB1 0x02
#define FPGA_SUB2 0x04

#define FPGA_RW_BIT(bit) (bit << 15)

#define FPGA_VOL_MASK(num)	(0x7 << (uint16_t)(3 * (num - 1)))
#define FPGA_VOL_BITS(num, vol)	((vol & 0x7) << (uint16_t)(3 * (num - 1)))

#define FPGA_WRITE_ENABLE_MASK	0x70
#define FPGA_WRITE_ENABLE_CHANNEL(num) (1 << (num + 4))
#define FPGA_WRITE_ENABLE_BIT	0x80

#define FPGA_KEY_PRESSED_BIT 0x100

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

	void writeReg(uint8_t reg, uint16_t value);
	uint16_t readReg(uint8_t reg);
	
	SPISettings settings;
	
	//for writing waves
	uint8_t writeChannel;
	uint8_t writeNum;
};

#endif