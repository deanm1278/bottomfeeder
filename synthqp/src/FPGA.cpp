#include "Arduino.h"
#include "FPGA.h"
#include "SPI.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

Q_DEFINE_THIS_FILE

FPGA::FPGA() :
QActive((QStateHandler)&FPGA::InitialPseudoState),
m_id(FPGA_ID), m_name("FPGA"), settings(4000000, MSBFIRST, SPI_MODE0),
m_startTimer(this, FPGA_START_TIMER) {}

FPGA::~FPGA() {}

void FPGA::Start(uint8_t prio) {
	pinMode(CDONE, INPUT);
	pinMode(CRESET_B, OUTPUT);
	digitalWrite(CRESET_B, LOW);
	
	pinMode(FPGA_CS, OUTPUT);
	digitalWrite(FPGA_CS, HIGH);
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState FPGA::InitialPseudoState(FPGA * const me, QEvt const * const e) {
	(void)e;
	
	me->m_deferQueue.init(me->m_deferQueueStor, ARRAY_COUNT(me->m_deferQueueStor));
	
	me->subscribe(FPGA_START_REQ);
	me->subscribe(FPGA_STOP_REQ);
	me->subscribe(FPGA_WRITE_WAVE_FILE);
	me->subscribe(FPGA_WRITE_PWM);
	me->subscribe(FPGA_WRITE_FS);
	me->subscribe(FPGA_WRITE_PARAM_REQ);
	me->subscribe(FPGA_NOTIFY_KEY_PRESSED);
	me->subscribe(FPGA_SET_PORTAMENTO_REQ);
	me->subscribe(FPGA_SET_ENABLE_REQ);
	me->subscribe(FPGA_WRITE_VOL_REQ);
	me->subscribe(FPGA_START_TIMER);
	
	me->subscribe(FLASH_CONFIG_READ_TO_LISTENER_DONE);
	
	return Q_TRAN(&FPGA::Root);
}

QState FPGA::Root(FPGA * const me, QEvt const * const e) {
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
			status = Q_TRAN(&FPGA::Stopped);
			break;
		}
		case FPGA_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FPGAStartCfm(req.GetSeq(), ERROR_STATE);
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

QState FPGA::Stopped(FPGA * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			digitalWrite(CRESET_B, LOW);
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			me->m_startTimer.disarm();
			status = Q_HANDLED();
			break;
		}
		case FPGA_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FPGAStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case FPGA_START_TIMER: {
			LOG_EVENT(e);
			
			if(digitalRead(CDONE)){
				SPI.begin();
				
				Evt const &req = EVT_CAST(*e);
				Evt *evt = new FPGAStartCfm(req.GetSeq(), ERROR_SUCCESS);
				QF::PUBLISH(evt, me);
				status = Q_TRAN(&FPGA::Started);
			}
			else{
				//TODO: fail here
				__BKPT();
			}
			break;
		}
		case FPGA_START_REQ: {
			LOG_EVENT(e);
			
			digitalWrite(CRESET_B, HIGH);
			
			me->m_startTimer.armX(1000);
			
			status = Q_HANDLED();
			
			break;
		}
		default: {
			status = Q_SUPER(&FPGA::Root);
			break;
		}
	}
	return status;
}

QState FPGA::Started(FPGA * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			
			//make sure the filter starts up
			me->writeReg(FPGA_PWM0, 4095);
			me->writeReg(FPGA_ENABLE, 0);
			me->writeReg(FPGA_W0_FREQ, 0);
			me->writeReg(FPGA_W1_FREQ, 0);
			me->writeReg(FPGA_W2_FREQ, 0);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case FPGA_WRITE_WAVE_FILE: {
			LOG_EVENT(e);
			FPGAWriteWaveFile const &req = static_cast<FPGAWriteWaveFile const &>(*e);
			
			me->writeNum = req.getnum();
			me->writeChannel = req.getChannel();
			
			status = Q_TRAN(&FPGA::WritingWave);
			
			break;
		}
		case FPGA_WRITE_PWM:{
			//LOG_EVENT(e);
			FPGAWritePWM const &req = static_cast<FPGAWritePWM const &>(*e);
			
			me->writeReg(req.getPwmNumber() + FPGA_PWM0, req.getValue());
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_WRITE_FS:{
			//LOG_EVENT(e);
			FPGAWriteFS const &req = static_cast<FPGAWriteFS const &>(*e);
			
			me->writeReg(FPGA_W0_FREQ + req.getChannel(), req.getFs());
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_WRITE_PARAM_REQ:{
			LOG_EVENT(e);
			FPGAWriteParamReq const &req = static_cast<FPGAWriteParamReq const &>(*e);
			
			uint8_t param = req.getParam();
			
			if(param == ATTACK || param == DECAY || param == RELEASE){
				uint32_t val = (uint32_t)(req.getValue() >> 16);
				me->writeReg(FPGA_A_INTERVAL_HIGH + param, val);
				
				val = (req.getValue() & 0xFFFF);
				me->writeReg(FPGA_A_INTERVAL_HIGH + param + 1, val);
			}
			else{
				me->writeReg(FPGA_A_INTERVAL_HIGH + param , req.getValue());
			}
			status = Q_HANDLED();
			break;
		}
		case FPGA_NOTIFY_KEY_PRESSED:{
			LOG_EVENT(e);
			FPGANotifyKeyPressed const &req = static_cast<FPGANotifyKeyPressed const &>(*e);

			uint16_t current = me->readReg(FPGA_ENABLE);
			
			if(req.getPressed()) current |= FPGA_KEY_PRESSED_BIT;
			else current &= ~FPGA_KEY_PRESSED_BIT;
			
			me->writeReg(FPGA_ENABLE, current);
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_SET_PORTAMENTO_REQ:{
			LOG_EVENT(e);
			FPGASetPortamentoReq const &req = static_cast<FPGASetPortamentoReq const &>(*e);

			me->writeReg(FPGA_PORT, req.getPrescale());
			status = Q_HANDLED();
			break;
		}
		case FPGA_SET_ENABLE_REQ: {
			LOG_EVENT(e);
			FPGASetEnableReq const &req = static_cast<FPGASetEnableReq const &>(*e);
			
			uint16_t current = me->readReg(FPGA_ENABLE);
			
			current &= ~0x0F;
			current |= req.getEnable();
			
			me->writeReg(FPGA_ENABLE, current);
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_WRITE_VOL_REQ: {
			LOG_EVENT(e);
			FPGAWriteVolReq const &req = static_cast<FPGAWriteVolReq const &>(*e);
			
			uint16_t current = me->readReg(FPGA_VOL);
			uint16_t toWrite = current &  ~(FPGA_VOL_MASK(req.getChannel()));
			toWrite |= FPGA_VOL_BITS(req.getChannel(), req.getVol());
			me->writeReg(FPGA_VOL, toWrite);
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FPGAStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&FPGA::Stopped);
			break;
		}
		default: {
			status = Q_SUPER(&FPGA::Root);
			break;
		}
	}
	return status;
}

QState FPGA::WritingWave(FPGA * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			
			uint16_t current = me->readReg(FPGA_ENABLE);
			uint16_t toWrite = current &  ~FPGA_WRITE_ENABLE_MASK;
						
			//set the channels to enable
			toWrite |= FPGA_WRITE_ENABLE_CHANNEL(me->writeChannel);
			toWrite |= FPGA_WRITE_ENABLE_BIT;
						
			me->writeReg(FPGA_ENABLE, toWrite);
						
			//request the flash config object to read us the wave
			Evt *evt = new FlashConfigReadToListenerReq(me->writeNum);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			
			//disable writing
			uint16_t current = me->readReg(FPGA_ENABLE);
						
			uint16_t toWrite = current & ~FPGA_WRITE_ENABLE_BIT;
			toWrite &= ~FPGA_WRITE_ENABLE_CHANNEL(me->writeChannel);
			me->writeReg(FPGA_ENABLE, toWrite);
			
			while(me->recall(&me->m_deferQueue));
			
			status = Q_HANDLED();
			break;
		}
		case FLASH_CONFIG_READ_TO_LISTENER_DONE: {
			LOG_EVENT(e);
			
			status = Q_TRAN(&FPGA::Started);
			
			break;
		}
		case FPGA_WRITE_FS:
		case FPGA_NOTIFY_KEY_PRESSED:
		case FPGA_WRITE_PWM:
		case FPGA_WRITE_PARAM_REQ:
		case FPGA_SET_PORTAMENTO_REQ:
		case FPGA_SET_ENABLE_REQ:
		case FPGA_WRITE_VOL_REQ:
		case FPGA_WRITE_WAVE_FILE:{
			LOG_EVENT(e);
			me->defer(&me->m_deferQueue, e);
			
			status = Q_HANDLED();
			break;
		}
		case Q_INIT_SIG: {
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&FPGA::Started);
			break;
		}
	}
	return status;
}

void FPGA::writeReg(uint8_t reg, uint16_t value){
	uint16_t Byte1 = FPGA_RW_BIT(1) | FPGA_REG_MASK(reg);
	
	QF_CRIT_ENTRY();
	SPI.beginTransaction (settings);
	digitalWrite(FPGA_CS, LOW);
	SPI.transfer16(Byte1);
	SPI.transfer16(value);
	digitalWrite(FPGA_CS, HIGH);
	SPI.endTransaction();
	QF_CRIT_EXIT();
}

uint16_t FPGA::readReg(uint8_t reg){
	uint16_t Byte1 = FPGA_RW_BIT(0) |FPGA_REG_MASK(reg);
	uint16_t read;
	
	QF_CRIT_ENTRY();
	SPI.beginTransaction (settings);
	digitalWrite(FPGA_CS, LOW);
	SPI.transfer16(Byte1);
	SPI.transfer16(0x00);
	
	digitalWrite(FPGA_CS, HIGH);
	
	digitalWrite(FPGA_CS, LOW);
	
	read = SPI.transfer16(0x00);
	digitalWrite(FPGA_CS, HIGH);
	SPI.endTransaction();
	QF_CRIT_EXIT();
	
	return read;
}