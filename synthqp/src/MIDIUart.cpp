/*!
 *  @file		MIDI.cpp
 *  Project		MIDI Library
 *	@brief		MIDI Library for the Arduino
 *	@version	3.2
 *  @author		Francois Best 
 *	@date		24/02/11
 *  license		GPL Forty Seven Effects - 2011
 */

#include <stdlib.h>
#include "Arduino.h"			// If using an old (pre-1.0) version of Arduino, use WConstants.h instead of Arduino.h
#include "HardwareSerial.h"
#include "MIDIUART.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

#define MIDI_BUF_MAX 256
static byte midiBuf[MIDI_BUF_MAX];
buffer MIDI_Class::MIDIBuf(midiBuf, MIDI_BUF_MAX);

/*! \brief Default constructor for MIDI_Class. */
MIDI_Class::MIDI_Class() :
QActive((QStateHandler)&MIDI_Class::InitialPseudoState),
m_id(MIDI_UART), m_name("MIDI_UART") {}
	
MIDI_Class::~MIDI_Class() {}

void MIDI_Class::Start(uint8_t prio) {
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
	
	// Initialise the Serial port
	USE_SERIAL_PORT.begin(MIDI_BAUDRATE);
		
	mInputChannel = MIDI_CHANNEL_OMNI;
	mRunningStatus_RX = InvalidType;
	mPendingMessageIndex = 0;
	mPendingMessageExpectedLenght = 0;
		
	mMessage.valid = false;
	mMessage.type = InvalidType;
	mMessage.channel = 0;
	mMessage.data1 = 0;
	mMessage.data2 = 0;
}

QState MIDI_Class::InitialPseudoState(MIDI_Class * const me, QEvt const * const e) {
	(void)e;
	
	me->subscribe(MIDI_UART_START_REQ);
	me->subscribe(MIDI_UART_STOP_REQ);
	me->subscribe(MIDI_UART_DATA_READY);
	
	return Q_TRAN(&MIDI_Class::Root);
}

QState MIDI_Class::Root(MIDI_Class * const me, QEvt const * const e) {
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
			status = Q_TRAN(&MIDI_Class::Stopped);
			break;
		}
		case MIDI_UART_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUARTStartCfm(req.GetSeq(), ERROR_STATE);
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

QState MIDI_Class::Stopped(MIDI_Class * const me, QEvt const * const e) {
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
		case MIDI_UART_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUARTStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case MIDI_UART_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUARTStartCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&MIDI_Class::Started);
			break;
		}
		default: {
			status = Q_SUPER(&MIDI_Class::Root);
			break;
		}
	}
	return status;
}

QState MIDI_Class::Started(MIDI_Class * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			//me->EnableRxInt();
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			//me->DisableRxInt();
			status = Q_HANDLED();
			break;
		}
		case MIDI_UART_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new MIDIUARTStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&MIDI_Class::Stopped);
			break;
		}
		case MIDI_UART_DATA_READY: {
			//LOG_EVENT(e);
			while(!MIDIBuf.empty()){
				me->read();
			}
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&MIDI_Class::Root);
			break;
		}
	}
	return status;
}

/*! \brief Read a MIDI message from the serial port using the main input channel (see setInputChannel() for reference).
 
 Returned value: true if any valid message has been stored in the structure, false if not.
 A valid message is a message that matches the input channel. \n\n
 If the Thru is enabled and the messages matches the filter, it is sent back on the MIDI output.
 */
bool MIDI_Class::read()
{
	
	return read(mInputChannel);
	
}


/*! \brief Reading/thru-ing method, the same as read() with a given input channel to read on. */
bool MIDI_Class::read(const byte inChannel)
{
	
	if (inChannel >= MIDI_CHANNEL_OFF) return false; // MIDI Input disabled.
	
	if (parse(inChannel)) {
		
		if (input_filter(inChannel)) {
			
			publish_midi_event();
			
			return true;
		}
		
	}
	
	return false;
	
}


// Private method: MIDI parser
bool MIDI_Class::parse(byte inChannel)
{ 
		
		/* Parsing algorithm:
		 Get a byte from the serial buffer.
		 * If there is no pending message to be recomposed, start a new one.
		 - Find type and channel (if pertinent)
		 - Look for other bytes in buffer, call parser recursively, until the message is assembled or the buffer is empty.
		 * Else, add the extracted byte to the pending message, and check validity. When the message is done, store it.
		 */
				
		const byte extracted = MIDIBuf.pop_front();
		
		if (mPendingMessageIndex == 0) { // Start a new pending message
			mPendingMessage[0] = extracted;
			
			// Check for running status first
			switch (getTypeFromStatusByte(mRunningStatus_RX)) {
					// Only these types allow Running Status:
				case NoteOff:
				case NoteOn:
				case AfterTouchPoly:
				case ControlChange:
				case ProgramChange:
				case AfterTouchChannel:
				case PitchBend:	
					
					// If the status byte is not received, prepend it to the pending message
					if (extracted < 0x80) {
						mPendingMessage[0] = mRunningStatus_RX;
						mPendingMessage[1] = extracted;
						mPendingMessageIndex = 1;
					}
					// Else: well, we received another status byte, so the running status does not apply here.
					// It will be updated upon completion of this message.
					
					break;
					
				default:
					// No running status
					break;
			}
			
			
			switch (getTypeFromStatusByte(mPendingMessage[0])) {
					
					// 1 byte messages
				case MIDIStart:
				case Continue:
				case Stop:
				case Clock:
				case ActiveSensing:
				case SystemReset:
				case TuneRequest:
					// Handle the message type directly here.
					mMessage.type = getTypeFromStatusByte(mPendingMessage[0]);
					mMessage.channel = 0;
					mMessage.data1 = 0;
					mMessage.data2 = 0;
					mMessage.valid = true;
					
					// \fix Running Status broken when receiving Clock messages.
					// Do not reset all input attributes, Running Status must remain unchanged.
					//reset_input_attributes(); 
					
					// We still need to reset these
					mPendingMessageIndex = 0;
					mPendingMessageExpectedLenght = 0;
					
					return true;
					break;
					
					// 2 bytes messages
				case ProgramChange:
				case AfterTouchChannel:
				case TimeCodeQuarterFrame:
				case SongSelect:
					mPendingMessageExpectedLenght = 2;
					break;
					
					// 3 bytes messages
				case NoteOn:
				case NoteOff:
				case ControlChange:
				case PitchBend:
				case AfterTouchPoly:
				case SongPosition:
					mPendingMessageExpectedLenght = 3;
					break;
					
				case SystemExclusive:
					mPendingMessageExpectedLenght = MIDI_SYSEX_ARRAY_SIZE; // As the message can be any lenght between 3 and MIDI_SYSEX_ARRAY_SIZE bytes
					mRunningStatus_RX = InvalidType;
					break;
					
				case InvalidType:
				default:
					// This is obviously wrong. Let's get the hell out'a here.
					reset_input_attributes();
					return false;
					break;
			}
			
			// Then update the index of the pending message.
			mPendingMessageIndex++;
			
#if USE_1BYTE_PARSING
			// Message is not complete.
			return false;
#else
			// Call the parser recursively
			// to parse the rest of the message.
			return parse(inChannel);
#endif
			
		}
		else { 
			
			// First, test if this is a status byte
			if (extracted >= 0x80) {
				
				// Reception of status bytes in the middle of an uncompleted message
				// are allowed only for interleaved Real Time message or EOX
				switch (extracted) {
					case Clock:
					case MIDIStart:
					case Continue:
					case Stop:
					case ActiveSensing:
					case SystemReset:
						
						/*
						 This is tricky. Here we will have to extract the one-byte message,
						 pass it to the structure for being read outside the MIDI class,
						 and recompose the message it was interleaved into.
						 
						 Oh, and without killing the running status.. 
						 
						 This is done by leaving the pending message as is, it will be completed on next calls.
						 */
						
						mMessage.type = (kMIDIType)extracted;
						mMessage.data1 = 0;
						mMessage.data2 = 0;
						mMessage.channel = 0;
						mMessage.valid = true;
						return true;
						
						break;
						
						// End of Exclusive
					case 0xF7:
						if (getTypeFromStatusByte(mPendingMessage[0]) == SystemExclusive) {
							
							// Store System Exclusive array in midimsg structure
							for (byte i=0;i<MIDI_SYSEX_ARRAY_SIZE;i++) {
								mMessage.sysex_array[i] = mPendingMessage[i];
							}
							
							mMessage.type = SystemExclusive;

							// Get length
							mMessage.data1 = (mPendingMessageIndex+1) & 0xFF;	
							mMessage.data2 = (mPendingMessageIndex+1) >> 8;
							
							mMessage.channel = 0;
							mMessage.valid = true;
							
							reset_input_attributes();
							
							return true;
						}
						else {
							// Well well well.. error.
							reset_input_attributes();
							return false;
						}
						
						break;
					default:
						break;
				}
				
				
				
			}
			
			
			// Add extracted data byte to pending message
			mPendingMessage[mPendingMessageIndex] = extracted;
			
			
			// Now we are going to check if we have reached the end of the message
			if (mPendingMessageIndex >= (mPendingMessageExpectedLenght-1)) {
				
				// "FML" case: fall down here with an overflown SysEx..
				// This means we received the last possible data byte that can fit the buffer.
				// If this happens, try increasing MIDI_SYSEX_ARRAY_SIZE.
				if (getTypeFromStatusByte(mPendingMessage[0]) == SystemExclusive) {
					reset_input_attributes();
					return false;
				}
				
				
				mMessage.type = getTypeFromStatusByte(mPendingMessage[0]);
				mMessage.channel = (mPendingMessage[0] & 0x0F)+1; // Don't check if it is a Channel Message
				
				mMessage.data1 = mPendingMessage[1];
				
				// Save data2 only if applicable
				if (mPendingMessageExpectedLenght == 3)	mMessage.data2 = mPendingMessage[2];
				else mMessage.data2 = 0;
				
				// Reset local variables
				mPendingMessageIndex = 0;
				mPendingMessageExpectedLenght = 0;
				
				mMessage.valid = true;
				
				// Activate running status (if enabled for the received type)
				switch (mMessage.type) {
					case NoteOff:
					case NoteOn:
					case AfterTouchPoly:
					case ControlChange:
					case ProgramChange:
					case AfterTouchChannel:
					case PitchBend:	
						// Running status enabled: store it from received message
						mRunningStatus_RX = mPendingMessage[0];
						break;
						
					default:
						// No running status
						mRunningStatus_RX = InvalidType;
						break;
				}
				return true;
			}
			else {
				// Then update the index of the pending message.
				mPendingMessageIndex++;
				
#if USE_1BYTE_PARSING
				// Message is not complete.
				return false;
#else
				// Call the parser recursively
				// to parse the rest of the message.
				return parse(inChannel);
#endif
				
			}
			
		}
	return false;
}


// Private method: check if the received message is on the listened channel
bool MIDI_Class::input_filter(byte inChannel)
{
	
	
	// This method handles recognition of channel (to know if the message is destinated to the Arduino)
	
	
	if (mMessage.type == InvalidType) return false;
	
	
	// First, check if the received message is Channel
	if (mMessage.type >= NoteOff && mMessage.type <= PitchBend) {
		
		// Then we need to know if we listen to it
		if ((mMessage.channel == mInputChannel) || (mInputChannel == MIDI_CHANNEL_OMNI)) {
			return true;
			
		}
		else {
			// We don't listen to this channel
			return false;
		}
		
	}
	else {
		
		// System messages are always received
		return true;
	}
	
}


// Private method: reset input attributes
void MIDI_Class::reset_input_attributes()
{
	
	mPendingMessageIndex = 0;
	mPendingMessageExpectedLenght = 0;
	mRunningStatus_RX = InvalidType;
	
}


// Getters
/*! \brief Get the last received message's type
 
 Returns an enumerated type. @see kMIDIType
 */
kMIDIType MIDI_Class::getType() const
{
	
	return mMessage.type;

}


/*! \brief Get the channel of the message stored in the structure.
 
 Channel range is 1 to 16. For non-channel messages, this will return 0.
 */
byte MIDI_Class::getChannel() const
{
	
	return mMessage.channel;

}


/*! \brief Get the first data byte of the last received message. */
byte MIDI_Class::getData1() const
{
	
	return mMessage.data1;

}


/*! \brief Get the second data byte of the last received message. */
byte MIDI_Class::getData2() const
{ 
	
	return mMessage.data2;

}


/*! \brief Get the System Exclusive byte array. 
 
 @see getSysExArrayLength to get the array's length in bytes.
 */
const byte * MIDI_Class::getSysExArray() const
{ 
	
	return mMessage.sysex_array;

}

/*! \brief Get the lenght of the System Exclusive array.
 
 It is coded using data1 as LSB and data2 as MSB.
 \return The array's length, in bytes.
 */
unsigned int MIDI_Class::getSysExArrayLength() const
{
	
	unsigned int coded_size = ((unsigned int)(mMessage.data2) << 8) | mMessage.data1;
	
	return (coded_size > MIDI_SYSEX_ARRAY_SIZE) ? MIDI_SYSEX_ARRAY_SIZE : coded_size;
	
}


/*! \brief Check if a valid message is stored in the structure. */
bool MIDI_Class::check() const
{ 
	
	return mMessage.valid;

}


// Setters
/*! \brief Set the value for the input MIDI channel 
 \param Channel the channel value. Valid values are 1 to 16, 
 MIDI_CHANNEL_OMNI if you want to listen to all channels, and MIDI_CHANNEL_OFF to disable MIDI input.
 */
void MIDI_Class::setInputChannel(const byte Channel)
{ 
	
	mInputChannel = Channel;
	
}

void MIDI_Class::RxCallback(uint8_t id) {
	MIDIBuf.push_back(USE_SERIAL_PORT.read());
	if(!MIDIBuf.empty()){
		//if this is the first event, send a data ready event
		Evt *evt = new Evt(MIDI_UART_DATA_READY);
		QF::PUBLISH(evt, 0);
	}
}

void MIDI_Class::publish_midi_event(){
	Evt *evt;
	
	//switch and send message based on received midi message
	switch (mMessage.type) {
		// Notes
		case NoteOff:{
			evt = new noteOffReq(mMessage.channel, mMessage.data1, mMessage.data2);
			break;
		}
		case NoteOn:{
			if(mMessage.data2 == 0) 
				evt = new noteOffReq(mMessage.channel, mMessage.data1, mMessage.data2);
			else
				evt = new noteOnReq(mMessage.channel,mMessage.data1,mMessage.data2);
			break;
		}
		case ControlChange: {
			evt = new controlChangeReq(mMessage.channel, mMessage.data1, mMessage.data2);
			break;
		}
		case PitchBend: {
			evt = new pitchBendReq(mMessage.channel,(int)((mMessage.data1 & 0x7F) | ((mMessage.data2 & 0x7F)<< 7)) - 8192); // TODO: check this
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
		case AfterTouchPoly:		if (mAfterTouchPolyCallback != NULL)		mAfterTouchPolyCallback(mMessage.channel,mMessage.data1,mMessage.data2);	break;
		case AfterTouchChannel:		if (mAfterTouchChannelCallback != NULL)		mAfterTouchChannelCallback(mMessage.channel,mMessage.data1);	break;
		
		case ProgramChange:			if (mProgramChangeCallback != NULL)			mProgramChangeCallback(mMessage.channel,mMessage.data1);	break;
		case SystemExclusive:		if (mSystemExclusiveCallback != NULL)		mSystemExclusiveCallback(mMessage.sysex_array,mMessage.data1);	break;
		
		// Occasional messages
		case TimeCodeQuarterFrame:	if (mTimeCodeQuarterFrameCallback != NULL)	mTimeCodeQuarterFrameCallback(mMessage.data1);	break;
		case SongPosition:			if (mSongPositionCallback != NULL)			mSongPositionCallback((mMessage.data1 & 0x7F) | ((mMessage.data2 & 0x7F)<< 7));	break;
		case SongSelect:			if (mSongSelectCallback != NULL)			mSongSelectCallback(mMessage.data1);	break;
		case TuneRequest:			if (mTuneRequestCallback != NULL)			mTuneRequestCallback();	break;
		
		case SystemReset:			if (mSystemResetCallback != NULL)			mSystemResetCallback();	break;
		case InvalidType:
		*/
		default:
		break;
	}
	QF::PUBLISH(evt, me);
}


