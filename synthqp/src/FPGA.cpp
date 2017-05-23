#include "Arduino.h"
#include "FPGA.h"
#include "SPI.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

Q_DEFINE_THIS_FILE

#define SD_READ_MAX 256
static byte sdbuf[SD_READ_MAX];

FPGA::FPGA() :
QActive((QStateHandler)&FPGA::InitialPseudoState),
m_id(FPGA_ID), m_name("FPGA"), settings(4000000, MSBFIRST, SPI_MODE0),
SDBuffer(sdbuf, SD_READ_MAX) {}

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
	
	me->subscribe(SD_READ_FILE_RESPONSE);
	
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
		case FPGA_START_REQ: {
			LOG_EVENT(e);
			
			digitalWrite(CRESET_B, HIGH);
			
			delay(500);
			
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
			
			me->writeChannel = req.getChannel();
			me->writeVolume = req.getVolume();
			me->writePos = 0;
			
			char path[50];
			strcpy(path, WAVES_PATH);
			strcat(path, req.getFilename());
			
			Evt const &r = EVT_CAST(*e);
			Evt *evt = new SDReadFileReq(0, path, 0, SD_READ_MAX, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
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
			
			me->writeReg(FPGA_A_INTERVAL + req.getParam(), req.getValue());
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_NOTIFY_KEY_PRESSED:{
			LOG_EVENT(e);
			FPGANotifyKeyPressed const &req = static_cast<FPGANotifyKeyPressed const &>(*e);

			me->writeReg(FPGA_KEY_PRESSED, req.getPressed());
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
			
			me->writeReg(FPGA_ENABLE, req.getEnable());
			
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
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			//recall any deferred events
			while(me->recall(&me->m_deferQueue));
			
			status = Q_HANDLED();
			break;
		}
		case FPGA_WRITE_WAVE_FILE: {
			//defer this, we will resend it on exit
			me->defer(&me->m_deferQueue, e);
			status = Q_HANDLED();
			break;
		}
		case SD_READ_FILE_RESPONSE: {
			LOG_EVENT(e);
			
			SDReadFileResponse const &req = static_cast<SDReadFileResponse const &>(*e);
			if(req.getBuf() == &me->SDBuffer){
				//we know this read is for us
				
				if(req.getError()){
					//oh well, lets just keep going it's fine
					status = Q_TRAN(&FPGA::Started);
				}
				else{
					signed short val;
					byte buf[2];
					while(!me->SDBuffer.empty()){
						buf[0] = me->SDBuffer.pop_front();
						buf[1] = me->SDBuffer.pop_front();

						uint16_t rawval = ((uint16_t)buf[1] << 8) | (uint16_t)buf[0];
					
						val = reinterpret_cast<signed short&>(rawval);
						
						//adjust volume and convert to unsigned
						uint32_t adj = map(val, -32768, 32767, 0 - me->writeVolume, me->writeVolume) + 32768;
						
						me->writeWaveSample(me->writeChannel, me->writePos, (uint16_t)adj);
						me->writePos++;
					}
				
					if(req.getEof()){
						Evt *evt = new Evt(FPGA_WRITE_WAVE_CFM);
						QF::PUBLISH(evt, me);
					
						status = Q_TRAN(&FPGA::Started);
					}
					else{
						//request more data
						Evt const &r = EVT_CAST(*e);
						Evt *evt = new SDReadFileReq(r.GetSeq(), req.getFilename(), req.getExitPos(), SD_READ_MAX, &me->SDBuffer);
						QF::PUBLISH(evt, me);
					
						status = Q_HANDLED();
					}
				}
			}
			break;
		}
		default: {
			status = Q_SUPER(&FPGA::Started);
			break;
		}
	}
	return status;
}

void FPGA::writeWaveSample(uint16_t channel, uint16_t sample, uint16_t value){
	uint16_t bank = floor(sample/256);
	uint16_t addr = sample%256;
	
	//SerialUSB.println(value);
	
	uint16_t Byte1 = (uint16_t)FPGA_RW_BIT(1) | (uint16_t)FPGA_WRITE_TYPE_BIT(1)
	| (uint16_t)FPGA_CHANNEL_BITS(channel)
	| (uint16_t)FPGA_BANK_BITS(bank) | (uint16_t)FPGA_ADDR_BITS(addr);
	
	QF_CRIT_ENTRY();
	SPI.beginTransaction (settings);
	digitalWrite(FPGA_CS, LOW);
	SPI.transfer16(Byte1);
	SPI.transfer16(value);
	digitalWrite(FPGA_CS, HIGH);
	SPI.endTransaction();
	QF_CRIT_EXIT();
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
	
	SPI.beginTransaction (settings);
	digitalWrite(FPGA_CS, LOW);
	SPI.transfer16(Byte1);
	SPI.transfer16(0x00);
	
	digitalWrite(FPGA_CS, HIGH);
	
	digitalWrite(FPGA_CS, LOW);
	
	read = SPI.transfer16(0x00);
	digitalWrite(FPGA_CS, HIGH);
	SPI.endTransaction();
	
	return read;
}