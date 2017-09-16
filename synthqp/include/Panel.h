#ifndef PANEL_H
#define PANEL_H

#include "Arduino.h"
#include "qpcpp.h"
#include "qp_extras.h"

#include "mcp3008.h"

#define PANEL_NUM_ADC 4
#define PANEL_NUM_ADC_CHANNELS 31

#define PANEL_CHANGE_THRESHOLD 3

using namespace QP;
using namespace FW;

class Panel : public QActive {
	
	public:
	Panel();
	~Panel();
	
	void Start(uint8_t prio);
	
	static void timerCallback();
	
	protected:
	static QState InitialPseudoState(Panel * const me, QEvt const * const e);
	static QState Root(Panel * const me, QEvt const * const e);
	static QState Started(Panel * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 16
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	uint8_t m_id;
	char const * m_name;
	
	bool m_paraMode;
	bool m_paraButtonState;
	
	static mcp3008 adcs[PANEL_NUM_ADC];
	uint16_t m_previousValues[PANEL_NUM_ADC_CHANNELS];
	
	void sendChange(uint8_t ix);
	
};

#endif