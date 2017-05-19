#ifndef LIB_MIDI_USB_H_
#define LIB_MIDI_USB_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "Arduino.h"
#include "MIDIUSB2.h"
#include "buffer.h"

#include "hsm_id.h"

using namespace QP;
using namespace FW;

class MIDIUSB : public QActive {
	
	public:
	MIDIUSB();
	~MIDIUSB();
	
	void Start(uint8_t prio);
	
	static void RxCallback();
	
	protected:
	static QState InitialPseudoState(MIDIUSB * const me, QEvt const * const e);
	static QState Root(MIDIUSB * const me, QEvt const * const e);
	static QState Stopped(MIDIUSB * const me, QEvt const * const e);
	static QState Started(MIDIUSB * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 16
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	uint8_t m_id;
	char const * m_name;
	
	static buffer MIDIBuf;
	static MIDI_ MidiUSBDevice;
	
	void publish_midi_event();
};

#endif