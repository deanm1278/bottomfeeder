#include "Arduino.h"			
#include "FlashConfig.h"
#include "SPI.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"
#include "waveDefs.h"

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
	
	me->subscribe(FLASH_CONFIG_WRITE_WAVEFORMS_REQ);
	
	me->subscribe(FLASH_CONFIG_READ_LFO_REQ);
	me->subscribe(FLASH_CONFIG_READ_TO_LISTENER_REQ);
	
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
		case FLASH_CONFIG_WRITE_WAVEFORMS_REQ: {
			LOG_EVENT(e);
			
			status = Q_TRAN(&FlashConfig::WritingWaveforms);
			break;
		}
		case FLASH_CONFIG_READ_LFO_REQ: {
			LOG_EVENT(e);
			FlashConfigReadLFOReq const &req = static_cast<FlashConfigReadLFOReq const &>(*e);
			
			signed short *b = req.getWriteTo();
			
			uint32_t read_addr = WAVEFORMS_OFFSET + (WAVEFORM_SIZE * req.getNum());
			uint16_t offset = 0;
			
			byte buf[256];
			
			for(int i=0; i<8; i++){
				
				//read 256 bytes from flash
				me->read(read_addr, buf, 256);
				
				//take every 4th byte
				for(int j=0; j<256; j+=sizeof(signed short) * 4){
					
					uint16_t x;
					signed short toWrite;
					memcpy(&x, &buf[j], sizeof(uint16_t));
					
					if(x & SIGN_BIT){
						toWrite = (signed short)0 - (signed short)(x & ~SIGN_BIT);
					}
					else toWrite = x;
					
					memcpy(b, &toWrite, sizeof(signed short));
					b++;
				}
				
				read_addr += 256;
			}
			
			status = Q_HANDLED();
			break;
		}
		case FLASH_CONFIG_READ_TO_LISTENER_REQ: {
			LOG_EVENT(e);
			
			FlashConfigReadToListenerReq const &req = static_cast<FlashConfigReadToListenerReq const &>(*e);
			uint32_t read_addr = WAVEFORMS_OFFSET + (WAVEFORM_SIZE * req.getNum());
			
			me->readToListener(read_addr, WAVEFORM_SIZE);
			
			Evt *evt = new Evt(FLASH_CONFIG_READ_TO_LISTENER_DONE);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
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
			for (uint32_t addr = 0; addr < WAVEFORMS_OFFSET; addr += 0x1000) {
				me->write_enable();
				me->flash_4kB_erase(addr);
				me->wait();
			}
			
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

QState FlashConfig::WritingWaveforms(FlashConfig * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			
			me->waveform_number = 0;
			
			while(me->waveform_number <= 127 && waveforms[me->waveform_number] == NULL)
			me->waveform_number++;

			me->write_addr = WAVEFORMS_OFFSET + (2048 * me->waveform_number);
			
			char path[50];
			strcpy(path, WAVES_PATH);
			strcat(path, waveforms[me->waveform_number]);
			
			me->flash_power_up();
			
			for (uint32_t addr = WAVEFORMS_OFFSET; addr < WAVEFORMS_OFFSET + (WAVEFORM_SIZE * 127); addr += 0x1000) {
				me->write_enable();
				me->flash_4kB_erase(addr);
				me->wait();
			}
			
			Evt const &r = EVT_CAST(*e);
			Evt *evt = new SDReadFileReq(r.GetSeq(), path, 0, SD_READ_MAX, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
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

				uint8_t wbuf[256];
				
				signed short val;
				byte buf[2];
				uint16_t pos = 0;
				while(!me->SDBuffer.empty()){
					buf[0] = me->SDBuffer.pop_front();
					buf[1] = me->SDBuffer.pop_front();
					
					//TODO: fix in waveform gen script so we dont need to convert to sign/mag
					uint16_t rawval = ((uint16_t)buf[1] << 8) | (uint16_t)buf[0];
					uint16_t toWrite = 0;
					
					val = reinterpret_cast<signed short&>(rawval);
					
					if(val == -32768) toWrite = 0xFFFF;
					else if(val < 0) toWrite = (0x8000 | abs(val));
					else toWrite = val;
					
					memcpy(wbuf + pos, &toWrite, sizeof(uint16_t));
					pos += sizeof(uint16_t);
				}

				me->write_enable();
				me->prog(me->write_addr, wbuf, req.getBytesRead());
				
				uint32_t exitPos = req.getExitPos();
				
				//request more data
				me->write_addr += req.getBytesRead();
				int page_size = 256 - (me->rw_offset + me->write_addr) % 256;
				
				if(req.getEof()){
					//get the next waveform
					me->waveform_number++;
					while(me->waveform_number <= 127 && waveforms[me->waveform_number] == NULL)
						me->waveform_number++;
						
					exitPos = 0;
					me->write_addr = WAVEFORMS_OFFSET + (WAVEFORM_SIZE * me->waveform_number);
				}
				
				if(me->waveform_number > 127){
					//we are at the end of the waveforms
					Evt *evt = new Evt(FLASH_CONFIG_WRITE_DONE);
					QF::PUBLISH(evt, me);
					
					status = Q_TRAN(&FlashConfig::Started);
				}
				else{
					//otherwise, get more data to write
					char path[50];
					strcpy(path, WAVES_PATH);
					strcat(path, waveforms[me->waveform_number]);
					
					Evt const &r = EVT_CAST(*e);
					Evt *evt = new SDReadFileReq(r.GetSeq(), path, exitPos, page_size, &me->SDBuffer);
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
	
	write_enable();
	
	SPI.beginTransaction(settings);
	
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0x81);
	SPI.transfer(0x8b);
	digitalWrite(SPI_SS_B, HIGH);
	
	SPI.endTransaction();
	
	SPI.beginTransaction(settings);
	
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(0x85);
	uint8_t d = SPI.transfer(0x00);
	digitalWrite(SPI_SS_B, HIGH);
	
	SPI.endTransaction();
	
	
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
	
	QF_CRIT_ENTRY();
	uint8_t command[4] = { 0x02, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(command, 4);
	SPI.transfer(data, n);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();
	QF_CRIT_EXIT();
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

	QF_CRIT_ENTRY();
	
	uint8_t command[4] = { 0x03, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(command, 4);
	memset(data, 0, n);
	SPI.transfer(data, n);
	digitalWrite(SPI_SS_B, HIGH);
	SPI.endTransaction();
	
	QF_CRIT_EXIT();

	#ifdef BFX_DEBUG
	for (int i = 0; i < n; i++){
		char k = (i == n-1 || i % 32 == 31 ? '\n' : ' ');
		SerialUSB.print(data[i], HEX);
		SerialUSB.print(k);
	}
	#endif
}

void FlashConfig::readToListener(uint32_t addr, uint16_t n)
{
		QF_CRIT_ENTRY();
		uint8_t command[4] = { 0x0B, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
		SPI.beginTransaction(settings);
		digitalWrite(SPI_SS_B, LOW);
		
		SPI.transfer(command, 4);
		
		SPI.transfer16(0x00);
		
		for(int i=0; i<n; i++){
			SPI.transfer(0x00);
		}
		
		digitalWrite(SPI_SS_B, HIGH);
		SPI.endTransaction();
		
		QF_CRIT_EXIT();
}

void FlashConfig::wait()
{
	#ifdef BFX_DEBUG
	SerialUSB.print("waiting..");
	#endif

	while (1)
	{
		uint8_t data[2] = { 0x05 };
			
		QF_CRIT_ENTRY();
		SPI.beginTransaction(settings);
		digitalWrite(SPI_SS_B, LOW);
		SPI.transfer(data, 2);
		digitalWrite(SPI_SS_B, HIGH);
		SPI.endTransaction();
		QF_CRIT_EXIT();

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

void FlashConfig::flash_4kB_erase(uint32_t addr)
{
	uint8_t command[4] = { 0x20, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
	SPI.beginTransaction(settings);
	digitalWrite(SPI_SS_B, LOW);
	SPI.transfer(command, 4);
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