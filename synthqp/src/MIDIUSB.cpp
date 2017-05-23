#include "Arduino.h"
#include "MIDIUSB.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

Q_DEFINE_THIS_FILE

#define MIDI_BUF_MAX 256
static byte midiBuf[MIDI_BUF_MAX];
buffer MIDIUSB::MIDIBuf(midiBuf, MIDI_BUF_MAX);

MIDI_ MIDIUSB::MidiUSBDevice;

MIDIUSB::MIDIUSB() :
QActive((QStateHandler)&MIDIUSB::InitialPseudoState),
m_id(MIDI_USB), m_name("MIDIUSB") {}

MIDIUSB::~MIDIUSB() {}
	
extern "C" void USB_handler(){
	QXK_ISR_ENTRY();
	USBDevice.ISRHandler();
	MIDIUSB::RxCallback();
	QXK_ISR_EXIT();
};

void MIDIUSB::Start(uint8_t prio) {
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState MIDIUSB::InitialPseudoState(MIDIUSB * const me, QEvt const * const e) {
	(void)e;
	
	me->subscribe(MIDI_USB_START_REQ);
	me->subscribe(MIDI_USB_STOP_REQ);
	me->subscribe(MIDI_USB_DATA_READY);
	
	return Q_TRAN(&MIDIUSB::Root);
}

QState MIDIUSB::Root(MIDIUSB * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case Q_INIT_SIG: {
			status = Q_TRAN(&MIDIUSB::Stopped);
			break;
		}
		case MIDI_USB_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUSBStartCfm(req.GetSeq(), ERROR_STATE);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&QHsm::top);
			break;
		}
	}
	return status;
}

QState MIDIUSB::Stopped(MIDIUSB * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case MIDI_USB_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUSBStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case MIDI_USB_START_REQ: {
			LOG_EVENT(e);

			if(1){
				Evt const &req = EVT_CAST(*e);
				Evt *evt = new MIDIUSBStartCfm(req.GetSeq(), ERROR_SUCCESS);
				QF::PUBLISH(evt, me);
				status = Q_TRAN(&MIDIUSB::Started);
			}
			else{
				//TODO: fail here
				__BKPT();
			}
			break;
		}
		default: {
			status = Q_SUPER(&MIDIUSB::Root);
			break;
		}
	}
	return status;
}

QState MIDIUSB::Started(MIDIUSB * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			
			//set the usb interrupt handler
			USB_SetHandler(USB_handler);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case MIDI_USB_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUSBStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&MIDIUSB::Stopped);
			break;
		}
		case MIDI_USB_DATA_READY:{
			//LOG_EVENT(e);
			//read all midi events.
			//TODO: make this only read a certain amount and then post lifo if there are more
			while(!me->MIDIBuf.empty()){
				uint8_t newevt[4];
				me->MIDIBuf.transfer_out(newevt, 4);
				
				if(me->MidiUSBDevice.read(newevt)){
					me->publish_midi_event();
				}
			}
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&MIDIUSB::Root);
			break;
		}
	}
	return status;
}

void MIDIUSB::RxCallback() {
	//TODO: this buffer is actually not thread safe
	if(MidiUSBDevice.available()){
		//push the new data to the buffer
		MIDIBuf.transfer_in(MidiUSBDevice.event, 4);
		if(MIDIBuf.getCount() == 4){
			//if this is the first event, send a data ready event
			Evt *evt = new Evt(MIDI_USB_DATA_READY);
			QF::PUBLISH(evt, 0);
		}
	}
}

void MIDIUSB::publish_midi_event(){
	Evt *evt;
	
	//switch and send message based on received midi message
	switch (MidiUSBDevice.getType()) {
		// Notes
		case NOTE_OFF:{
			evt = new noteOffReq(MidiUSBDevice.getChannel(), MidiUSBDevice.getData1(), MidiUSBDevice.getData2());
			QF::PUBLISH(evt, me);
			break;
		}
		case NOTE_ON:{
			if(MidiUSBDevice.getData2() == 0) 
				evt = new noteOffReq(MidiUSBDevice.getChannel(), MidiUSBDevice.getData1(), MidiUSBDevice.getData2());
			else
				evt = new noteOnReq(MidiUSBDevice.getChannel(),MidiUSBDevice.getData1(),MidiUSBDevice.getData2());
			
			QF::PUBLISH(evt, me);
			break;
		}
		case CONTROL_CHANGE: {
			evt = new controlChangeReq(MidiUSBDevice.getChannel(), MidiUSBDevice.getData1(), MidiUSBDevice.getData2());	
			QF::PUBLISH(evt, me);
			break;
		}
		case PITCH_BEND: {
			evt = new pitchBendReq(MidiUSBDevice.getChannel(),(int)((MidiUSBDevice.getData1() & 0x7F) | ((MidiUSBDevice.getData2() & 0x7F)<< 7)) - 8192); // TODO: check this
			QF::PUBLISH(evt, me);
			break;
		}
		
		/* TODO: others
		// Real-time messages
		case Clock:					if (mClockCallback != NULL)					mClockCallback();			break;
		case Start:					if (mStartCallback != NULL)					mStartCallback();			break;
		case Continue:				if (mContinueCallback != NULL)				mContinueCallback();		break;
		case Stop:					if (mStopCallback != NULL)					mStopCallback();			break;
		case ActiveSensing:			if (mActiveSensingCallback != NULL)			mActiveSensingCallback();	break;
		
		// Continuous controllers
		case AfterTouchPoly:		if (mAfterTouchPolyCallback != NULL)		mAfterTouchPolyCallback(MidiUSBDevice.getChannel(),MidiUSBDevice.getData1(),MidiUSBDevice.getData2());	break;
		case AfterTouchChannel:		if (mAfterTouchChannelCallback != NULL)		mAfterTouchChannelCallback(MidiUSBDevice.getChannel(),MidiUSBDevice.getData1());	break;
		
		case ProgramChange:			if (mProgramChangeCallback != NULL)			mProgramChangeCallback(MidiUSBDevice.getChannel(),MidiUSBDevice.getData1());	break;
		case SystemExclusive:		if (mSystemExclusiveCallback != NULL)		mSystemExclusiveCallback(mMessage.sysex_array,MidiUSBDevice.getData1());	break;
		
		// Occasional messages
		case TimeCodeQuarterFrame:	if (mTimeCodeQuarterFrameCallback != NULL)	mTimeCodeQuarterFrameCallback(MidiUSBDevice.getData1());	break;
		case SongPosition:			if (mSongPositionCallback != NULL)			mSongPositionCallback((MidiUSBDevice.getData1() & 0x7F) | ((MidiUSBDevice.getData2() & 0x7F)<< 7));	break;
		case SongSelect:			if (mSongSelectCallback != NULL)			mSongSelectCallback(MidiUSBDevice.getData1());	break;
		case TuneRequest:			if (mTuneRequestCallback != NULL)			mTuneRequestCallback();	break;
		
		case SystemReset:			if (mSystemResetCallback != NULL)			mSystemResetCallback();	break;
		case InvalidType:
		*/
		default:
		break;
	}
}