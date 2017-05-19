#ifndef LIB_CAPTOUCH_H_
#define LIB_CAPTOUCH_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "Arduino.h"
#include "AD7147.h"

#include "hsm_id.h"

#define CT_CS A5
#define INT_PIN 8

using namespace QP;
using namespace FW;

class CapTouch : public QActive {
	
public:
	CapTouch();
	~CapTouch();
	
	void Start(uint8_t prio);
	
	static void touchCallback();
	
protected:
	static QState InitialPseudoState(CapTouch * const me, QEvt const * const e);
	static QState Root(CapTouch * const me, QEvt const * const e);
	static QState Stopped(CapTouch * const me, QEvt const * const e);
	static QState Started(CapTouch * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 16,
		DEFER_QUEUE_COUNT = 4
	};

	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	QEvt const *m_deferQueueStor[DEFER_QUEUE_COUNT];
	QEQueue m_deferQueue;
	uint8_t m_id;
	char const * m_name;
	
	uint16_t keyboardOffset;
	
private:
	static AD7147 ad7147;
	uint16_t oldBtns = 0;
};

#endif