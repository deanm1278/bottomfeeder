#ifndef LIB_SD_CARD_H_
#define LIB_SD_CARD_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"

#include "hsm_id.h"
#include "Arduino.h"

#include "SdFat.h"

#define SD_CS 12

#define CHUNK_SIZE 256

using namespace QP;
using namespace FW;

class SDCard : public QActive {
	
public:
	SDCard();
	~SDCard();
	
	void Start(uint8_t prio);
	
protected:
	static QState InitialPseudoState(SDCard * const me, QEvt const * const e);
	static QState Root(SDCard * const me, QEvt const * const e);
	static QState Stopped(SDCard * const me, QEvt const * const e);
	static QState Started(SDCard * const me, QEvt const * const e);

	enum {
		EVT_QUEUE_COUNT = 16
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	uint8_t m_id;
	char const * m_name;
	
private:
	SdFat SD;
};

#endif