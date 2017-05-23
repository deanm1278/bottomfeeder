#include "Arduino.h"			
#include "FlashConfig.h"
#include "SPI.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

Q_DEFINE_THIS_FILE

//#define BFX_DEBUG
#define SD_READ_MAX 256
static byte sdbuf[SD_READ_MAX];

FlashConfig::FlashConfig() :
QActive((QStateHandler)&FlashConfig::InitialPseudoState),
m_id(FLASH_CONFIG), m_name("FLASH_CONFIG"), settings(50000000, MSBFIRST, SPI_MODE0),
SDBuffer(sdbuf, SD_READ_MAX) {}

FlashConfig::~FlashConfig() {}
	
void FlashConfig::Start(uint8_t prio) {
	pinMode(SPI_SS_B, OUTPUT);
	digitalWrite(SPI_SS_B, HIGH);
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState FlashConfig::InitialPseudoState(FlashConfig * const me, QEvt const * const e) {
	(void)e;
	
	me->subscribe(FLASH_CONFIG_START_REQ);
	me->subscribe(FLASH_CONFIG_STOP_REQ);
	me->subscribe(FLASH_CONFIG_WRITE_CONFIGURATION);
	me->subscribe(FLASH_CONFIG_VERIFY_CONFIGURATION_REQ);
	
	me->subscribe(SD_READ_FILE_RESPONSE);
	
	return Q_TRAN(&FlashConfig::Root);
}

QState FlashConfig::Root(FlashConfig * const me, QEvt const * const e) {
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
			status = Q_TRAN(&FlashConfig::Stopped);
			break;
		}
		case FLASH_CONFIG_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FlashConfigStartCfm(req.GetSeq(), ERROR_STATE);
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

QState FlashConfig::Stopped(FlashConfig * const me, QEvt const * const e) {
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
		case FLASH_CONFIG_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FlashConfigStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case FLASH_CONFIG_START_REQ: {
			LOG_EVENT(e);
			SPI.begin();
			
			if(me->flash_read_id()){
				Evt const &req = EVT_CAST(*e);
				Evt *evt = new FlashConfigStartCfm(req.GetSeq(), ERROR_SUCCESS);
				QF::PUBLISH(evt, me);
				status = Q_TRAN(&FlashConfig::Started);
			}
			else{
				//TODO: fail here
				__BKPT();
			}
			break;
		}
		default: {
			status = Q_SUPER(&FlashConfig::Root);
			break;
		}
	}
	return status;
}

QState FlashConfig::Started(FlashConfig * const me, QEvt const * const e) {
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
		case FLASH_CONFIG_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new FlashConfigStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&FlashConfig::Stopped);
			break;
		}
		case FLASH_CONFIG_WRITE_CONFIGURATION: {
			LOG_EVENT(e);
			FlashConfigWriteConfiguration const &req = static_cast<FlashConfigWriteConfiguration const &>(*e);
			
			Evt const &r = EVT_CAST(*e);
			Evt *evt = new SDReadFileReq(r.GetSeq(), (char *)req.getFwPath(), 0, SD_READ_MAX, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
			status = Q_TRAN(&FlashConfig::WritingConfiguration);
			break;
		}
		case FLASH_CONFIG_VERIFY_CONFIGURATION_REQ: {
			LOG_EVENT(e);
			FlashConfigVerifyConfigurationReq const &req = static_cast<FlashConfigVerifyConfigurationReq const &>(*e);
			
			Evt const &r = EVT_CAST(*e);
			Evt *evt = new SDReadFileReq(r.GetSeq(), (char *)req.getFwPath(), 0, SD_READ_MAX, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
			status = Q_TRAN(&FlashConfig::VerifyConfiguration);
			break;
		}
		default: {
			status = Q_SUPER(&FlashConfig::Root);
			break;
		}
	}
	return status;
}

QState FlashConfig::WritingConfiguration(FlashConfig * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->write_addr = 0;
			me->flash_power_up();
			
			me->write_enable();
			me->bulk_erase();
			me->wait();
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			me->flash_power_down();
			status = Q_HANDLED();
			break;
		}
		case SD_READ_FILE_RESPONSE: {
			//LOG_EVENT(e);
			
			SDReadFileResponse const &req = static_cast<SDReadFileResponse const &>(*e);
			if(req.getBuf() == &me->SDBuffer){
				//we know this read is for us
				me->wait();
				
				uint8_t buf[256];
				me->SDBuffer.transfer_out(buf, req.getBytesRead());

				me->write_enable();
				me->prog(me->write_addr, buf, req.getBytesRead());
				
				if(req.getEof()){
					Evt *evt = new Evt(FLASH_CONFIG_WRITE_DONE);
					QF::PUBLISH(evt, me);
					
					status = Q_TRAN(&FlashConfig::Started);
				}
				else{
					//request more data
					me->write_addr += req.getBytesRead();
					int page_size = 256 - (me->rw_offset + me->write_addr) % 256;
					
					Evt const &r = EVT_CAST(*e);
					Evt *evt = new SDReadFileReq(r.GetSeq(), req.getFilename(), req.getExitPos(), page_size, &me->SDBuffer);
					QF::PUBLISH(evt, me);
				
					status = Q_HANDLED();
				}
			}
			break;
		}
		default: {
			status = Q_SUPER(&FlashConfig::Root);
			break;
		}
	}
	return status;
}

QState FlashConfig::VerifyConfiguration(FlashConfig * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->write_addr = 0;
			me->flash_power_up();
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			me->flash_power_down();
			status = Q_HANDLED();
			break;
		}
		case SD_READ_FILE_RESPONSE: {
			//LOG_EVENT(e);
			
			SDReadFileResponse const &req = static_cast<SDReadFileResponse const &>(*e);
			if(req.getBuf() == &me->SDBuffer){
				//we know this read is for us
				me->wait();
				
				uint8_t buf[256];
				uint8_t vbuf[256];
				me->SDBuffer.transfer_out(buf, req.getBytesRead());
				me->read(me->rw_offset + me->write_addr, vbuf, req.getBytesRead());
				Q_ASSERT(!memcmp(buf, vbuf, req.getBytesRead()));
				
				if(req.getEof()){
					status = Q_TRAN(&FlashConfig::Started);
				}
				else{
					//request more data
					me->write_addr += req.getBytesRead();
					int page_size = 256 - (me->rw_offset + me->write_addr) % 256;
					
					Evt const &r = EVT_CAST(*e);
					Evt *evt = new SDReadFileReq(r.GetSeq(), req.getFilename(), req.getExitPos(), page_size, &me->SDBuffer);
					QF::PUBLISH(evt, me);
					
					status = Q_HANDLED();
				}
			}
			break;
		}
		default: {
			status = Q_SUPER(&FlashConfig::Root);
			break;
		}
	}
	return status;
}

bool FlashConfig::flash_read_id()
{
	
	flash_power_up();
	
	uint8_t data[21] = { 0x9F };
		
	SPI.beginTransaction(settings);
	
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(data, 21);
	digitalWrite(SPI_SS_B, HIGH);
	
	SPI.endTransaction();
	
	flash_power_down();
	
	//make sure the received bytes match what the datasheet says they should be
	if(data[1] == 0x20 && data[2] == 0xBA && data[3] == 0x16 && data[4] == 0x10)
		return true;
		
	else return false;
}

void FlashConfig::prog(uint32_t addr, uint8_t *data, int n)
{
	#ifdef BFX_DEBUG
	SerialUSB.print("prog 0x");
	SerialUSB.print(addr, HEX);
	SerialUSB.print(" +0x");
	SerialUSB.print(n, HEX);
	SerialUSB.println("..");
	
	for (int i = 0; i < n; i++){
		char k = (i == n-1 || i % 32 == 31 ? '\n' : ' ');
		SerialUSB.print(data[i], HEX);
		SerialUSB.print(k);
	}
	#endif
	
	uint8_t command[4] = { 0x02, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(command, 4);
	SPI.transfer(data, n);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();
}

void FlashConfig::read(uint32_t addr, uint8_t *data, int n)
{
	#ifdef BFX_DEBUG
	SerialUSB.print("read 0x");
	SerialUSB.print(addr, HEX);
	SerialUSB.print(" +0x");
	SerialUSB.print(n, HEX);
	SerialUSB.println("..");
	#endif

	uint8_t command[4] = { 0x03, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(command, 4);
	memset(data, 0, n);
	SPI.transfer(data, n);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();

	#ifdef BFX_DEBUG
	for (int i = 0; i < n; i++){
		char k = (i == n-1 || i % 32 == 31 ? '\n' : ' ');
		SerialUSB.print(data[i], HEX);
		SerialUSB.print(k);
	}
	#endif
}

void FlashConfig::wait()
{
	#ifdef BFX_DEBUG
	SerialUSB.print("waiting..");
	#endif

	while (1)
	{
		uint8_t data[2] = { 0x05 };

		SPI.beginTransaction(settings);
		digitalWrite(SPI_SS_B, LOW);
		SPI.transfer(data, 2);
		digitalWrite(SPI_SS_B, HIGH);
		SPI.endTransaction();

		if ((data[1] & 0x01) == 0)
		break;

		#ifdef BFX_DEBUG
		SerialUSB.print(".");
		#endif
		delay(100);
	}

	#ifdef BFX_DEBUG
	SerialUSB.println();
	#endif
}

void FlashConfig::write_enable()
{
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0x06);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();
}

void FlashConfig::bulk_erase()
{
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0xC7);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();
}

void FlashConfig::flash_power_up()
{
	SPI.beginTransaction(settings);
	
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0xAB);
	digitalWrite(SPI_SS_B, HIGH);
	
	SPI.endTransaction();
}

void FlashConfig::flash_power_down()
{
	SPI.beginTransaction(settings);
		
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0xB9);
	digitalWrite(SPI_SS_B, HIGH);
  
	SPI.endTransaction();
}