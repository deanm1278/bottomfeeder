#include "Arduino.h"
#include "CapTouch.h"
#include "SPI.h"
#include "bsp.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

Q_DEFINE_THIS_FILE

#define  SENSOR_THRESH	12000
#define	 SENSOR_MAX		20000

AD7147 CapTouch::ad7147;

CapTouch::CapTouch() :
QActive((QStateHandler)&CapTouch::InitialPseudoState),
m_id(CAP_TOUCH), m_name("CapTouch"), keyboardOffset(36) {}

CapTouch::~CapTouch() {}

void CapTouch::Start(uint8_t prio) {
		pinMode(CT_CS, OUTPUT);
		digitalWrite(CT_CS, HIGH);
		ad7147.shutdown();
		QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState CapTouch::InitialPseudoState(CapTouch * const me, QEvt const * const e) {
	(void)e;
	
	me->m_deferQueue.init(me->m_deferQueueStor, ARRAY_COUNT(me->m_deferQueueStor));
	
	me->subscribe(CAP_TOUCH_START_REQ);
	me->subscribe(CAP_TOUCH_STOP_REQ);
	
	me->subscribe(CAP_TOUCH_TOUCHED);
	
	return Q_TRAN(&CapTouch::Root);
}

QState CapTouch::Root(CapTouch * const me, QEvt const * const e) {
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
			status = Q_TRAN(&CapTouch::Stopped);
			break;
		}
		case CAP_TOUCH_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new CapTouchStartCfm(req.GetSeq(), ERROR_STATE);
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

QState CapTouch::Stopped(CapTouch * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->ad7147.shutdown();
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case CAP_TOUCH_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new CapTouchStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case CAP_TOUCH_START_REQ: {
			LOG_EVENT(e);
			
			if(me->ad7147.begin(CT_CS, INT_PIN)){
				
				Evt const &req = EVT_CAST(*e);
				Evt *evt = new CapTouchStartCfm(req.GetSeq(), ERROR_SUCCESS);
				QF::PUBLISH(evt, me);
				status = Q_TRAN(&CapTouch::Started);
			}
			else{
				//TODO: fail here
				__BKPT();
			}
			break;
		}
		default: {
			status = Q_SUPER(&CapTouch::Root);
			break;
		}
	}
	return status;
}

QState CapTouch::Started(CapTouch * const me, QEvt const * const e) {
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
		case CAP_TOUCH_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new CapTouchStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&CapTouch::Stopped);
			break;
		}
		case CAP_TOUCH_TOUCHED: {
			//LOG_EVENT(e);
			uint16_t newNote;
			Evt *evt;
			
			for(int i=0; i<NUMBER_OF_SENSORS; i++){
				uint16_t val = (1 << i);
				if( !(me->oldBtns & val) && (me->ad7147.ButtonStatus & val) ){
					evt = new noteOnReq(1, me->keyboardOffset + i, 127);
					QF::PUBLISH(evt, me);
				}
				else if( (me->oldBtns & val) && !(me->ad7147.ButtonStatus & val) ){
					evt = new noteOffReq(1, me->keyboardOffset + i, 0);
					QF::PUBLISH(evt, me);
				}
			}
			
			me->oldBtns = me->ad7147.ButtonStatus;
			
			//enable interrupt
			EExt_Interrupts in = g_APinDescription[INT_PIN].ulExtInt;
			EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << in);
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&CapTouch::Root);
			break;
		}
	}
	return status;
}

void CapTouch::touchCallback(){
	if(ad7147.update()){
		Evt *evt = new Evt(CAP_TOUCH_TOUCHED);
		QF::PUBLISH(evt, me);
	}
	else{
		//enable interrupt
		EExt_Interrupts in = g_APinDescription[INT_PIN].ulExtInt;
		EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << in);
	}
}