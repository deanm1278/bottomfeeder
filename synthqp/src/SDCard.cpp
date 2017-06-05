#include "Arduino.h"			
#include "SDCard.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

#include "SdFat.h"

Q_DEFINE_THIS_FILE

static byte readBuf[CHUNK_SIZE];

SDCard::SDCard() :
QActive((QStateHandler)&SDCard::InitialPseudoState),
m_id(SD_CARD), m_name("SD"), SD() {}

SDCard::~SDCard() {}
	
void SDCard::Start(uint8_t prio) {
	pinMode(SD_CS, OUTPUT);
	digitalWrite(SD_CS, HIGH);
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState SDCard::InitialPseudoState(SDCard * const me, QEvt const * const e) {
	(void)e;
	
	me->subscribe(SD_START_REQ);
	me->subscribe(SD_STOP_REQ);
	
	me->subscribe(SD_READ_FILE_REQ);
	me->subscribe(SD_WRITE_FILE_REQ);
	
	return Q_TRAN(&SDCard::Root);
}

QState SDCard::Root(SDCard * const me, QEvt const * const e) {
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
			status = Q_TRAN(&SDCard::Stopped);
			break;
		}
		case SD_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SDStartCfm(req.GetSeq(), ERROR_STATE);
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

QState SDCard::Stopped(SDCard * const me, QEvt const * const e) {
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
		case SD_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SDStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case SD_START_REQ: {
			LOG_EVENT(e);
			if (me->SD.begin(SD_CS, SD_SCK_MHZ(50))) {
				Evt const &req = EVT_CAST(*e);
				Evt *evt = new SDStartCfm(req.GetSeq(), ERROR_SUCCESS);
				QF::PUBLISH(evt, me);
				status = Q_TRAN(&SDCard::Started);
			}
			else{
				//TODO: should go to fail here
				//status = Q_TRAN(&SDCard::Started);
				__BKPT();
			}
			break;
		}
		default: {
			status = Q_SUPER(&SDCard::Root);
			break;
		}
	}
	return status;
}

QState SDCard::Started(SDCard * const me, QEvt const * const e) {
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
		case SD_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SDStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&SDCard::Stopped);
			break;
		}
		case SD_READ_FILE_REQ: {
			//LOG_EVENT(e);
			
			//toggle LED to show card activity
			digitalWrite(13, !digitalRead(13));
			
			SdFile datafile;
			uint16_t bytes_read = 0;
			uint16_t this_read = 0;
			uint16_t to_read = 0;
			bool eof = false;
			bool error = false;
			
			SDReadFileReq const &req = static_cast<SDReadFileReq const &>(*e);
			
			uint32_t exit_pos = req.getPos();

			//read bytes in chunks until either end of file or requested number of bytes has been read
			while(bytes_read < req.getNumBytes() && !eof && !error){
				QF_CRIT_ENTRY();
				//QF_INT_DISABLE();
				
				char fn[50];
				strcpy(fn, req.getFilename());
				
				if(!datafile.open((const char*)fn)){
					__BKPT();
					error = true;
				}
				else {
					datafile.seekSet(exit_pos);
					
					if(datafile.available()){
						//don't overshoot the requested number of bytes
						to_read = (CHUNK_SIZE > req.getNumBytes() - bytes_read ? req.getNumBytes() - bytes_read : CHUNK_SIZE);
						
						this_read = datafile.read(&readBuf, to_read);
						
						//push to the passed buffer
						req.getBuf()->transfer_in(readBuf, this_read);
						
						bytes_read += this_read;
					}
					else eof = true;
					
					exit_pos = datafile.curPosition();
					datafile.close();
				}
				
				QF_CRIT_EXIT();
				//QF_INT_ENABLE();
			}
			
			//publish response that will be handled by whoever requested the data
			Evt const &resp = EVT_CAST(*e);
			Evt *evt = new SDReadFileResponse(resp.GetSeq(), req.getFilename(), eof, exit_pos, bytes_read, req.getBuf(), error);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case SD_WRITE_FILE_REQ: {
			//LOG_EVENT(e);
			
			//toggle LED to show card activity
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
			
			SdFile datafile;
			uint16_t bytes_written = 0;
			uint16_t to_write = 0;
			bool error = false;
			
			SDReadFileReq const &req = static_cast<SDReadFileReq const &>(*e);
			
			uint32_t exit_pos = req.getPos();

			//read bytes in chunks until either end of file or requested number of bytes has been read
			while(bytes_written < req.getNumBytes() && !error){
				QF_CRIT_ENTRY();
				//QF_INT_DISABLE();
				
				char fn[50];
				strcpy(fn, req.getFilename());
				
				if(!datafile.open((const char*)fn, O_CREAT | O_WRITE)){
					__BKPT();
					error = true;
				}
				else {
					if(exit_pos == 0) datafile.truncate(0);
					else datafile.seekSet(exit_pos);
					
					//don't overshoot the requested number of bytes
					to_write = (CHUNK_SIZE > req.getNumBytes() - bytes_written ? req.getNumBytes() - bytes_written : CHUNK_SIZE);
					
					req.getBuf()->transfer_out(readBuf, to_write);
					Q_ASSERT(datafile.write(&readBuf, to_write) == to_write);
						
					bytes_written += to_write;
					
					exit_pos = datafile.curPosition();
					datafile.close();
				}
				
				QF_CRIT_EXIT();
				//QF_INT_ENABLE();
			}
			
			//publish response that will be handled by whoever requested the write
			Evt const &resp = EVT_CAST(*e);
			Evt *evt = new SDWriteFileResponse(req.GetSeq() + 1, req.getFilename(), exit_pos, req.getBuf(), error);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&SDCard::Root);
			break;
		}
	}
	return status;
}