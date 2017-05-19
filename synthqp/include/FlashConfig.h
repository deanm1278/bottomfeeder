#ifndef LIB_FLASH_CONFIG_H_
#define LIB_FLASH_CONFIG_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "Arduino.h"
#include "SPI.h"

#include "buffer.h"
#include "hsm_id.h"

#define SPI_SS_B        A0

using namespace QP;
using namespace FW;

class FlashConfig : public QActive {
	
public:
	FlashConfig();
	~FlashConfig();
	
	void Start(uint8_t prio);
	
protected:
	static QState InitialPseudoState(FlashConfig * const me, QEvt const * const e);
	static QState Root(FlashConfig * const me, QEvt const * const e);
	static QState Stopped(FlashConfig * const me, QEvt const * const e);
	static QState Started(FlashConfig * const me, QEvt const * const e);
	static QState WritingConfiguration(FlashConfig * const me, QEvt const * const e);
	static QState VerifyConfiguration(FlashConfig * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 16
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	uint8_t m_id;
	char const * m_name;
	
private:
	int rw_offset = 0;
	
	uint32_t write_addr;
	
	void prog(uint32_t addr, uint8_t *data, int n);
	void read(uint32_t addr, uint8_t *data, int n);
	void wait();
	void write_enable();
	void bulk_erase();
	bool flash_read_id();
	void flash_power_up();
	void flash_power_down();
	
	SPISettings settings;
	
	buffer SDBuffer;
};

#endif