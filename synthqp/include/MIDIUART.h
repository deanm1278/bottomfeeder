/*!
 *  @file		MIDI.h
 *  Project		MIDI Library
 *	@brief		MIDI Library for the Arduino
 *	Version		3.2
 *  @author		Francois Best 
 *	@date		24/02/11
 *  License		GPL Forty Seven Effects - 2011
 */

#ifndef LIB_MIDI_H_
#define LIB_MIDI_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "buffer.h"

#include "hsm_id.h"

#define USE_SERIAL_PORT         Serial      // Change the number (to Serial1 for example) if you want
                                            // to use a different serial port for MIDI I/O.
#define USE_RUNNING_STATUS		1			// Running status enables short messages when sending multiple values
                                            // of the same type and channel.
                                            // Set to 0 if you have troubles with controlling you hardware
#define USE_1BYTE_PARSING       1           // Each call to MIDI.read will only parse one byte (might be faster).

#define MIDI_BAUDRATE			31250

#define MIDI_CHANNEL_OMNI		0
#define MIDI_CHANNEL_OFF		17			// and over

#define MIDI_SYSEX_ARRAY_SIZE	255			// Maximum size is 65535 bytes.

/*! Type definition for practical use (because "unsigned char" is a bit long to write.. )*/
typedef uint8_t byte;
//typedef uint16_t word;  // unused, causes problems on 32 bit ARM boards

using namespace QP;
using namespace FW;

/*! Enumeration of MIDI types */
enum kMIDIType {
	NoteOff	              = 0x80,	///< Note Off
	NoteOn                = 0x90,	///< Note On
	AfterTouchPoly        = 0xA0,	///< Polyphonic AfterTouch
	ControlChange         = 0xB0,	///< Control Change / Channel Mode
	ProgramChange         = 0xC0,	///< Program Change
	AfterTouchChannel     = 0xD0,	///< Channel (monophonic) AfterTouch
	PitchBend             = 0xE0,	///< Pitch Bend
	SystemExclusive       = 0xF0,	///< System Exclusive
	TimeCodeQuarterFrame  = 0xF1,	///< System Common - MIDI Time Code Quarter Frame
	SongPosition          = 0xF2,	///< System Common - Song Position Pointer
	SongSelect            = 0xF3,	///< System Common - Song Select
	TuneRequest           = 0xF6,	///< System Common - Tune Request
	Clock                 = 0xF8,	///< System Real Time - Timing Clock
	MIDIStart             = 0xFA,	///< System Real Time - Start
	Continue              = 0xFB,	///< System Real Time - Continue
	Stop                  = 0xFC,	///< System Real Time - Stop
	ActiveSensing         = 0xFE,	///< System Real Time - Active Sensing
	SystemReset           = 0xFF,	///< System Real Time - System Reset
	InvalidType           = 0x00    ///< For notifying errors
};

/*! Enumeration of Thru filter modes */
enum kThruFilterMode {
	Off                   = 0,  ///< Thru disabled (nothing passes through).
	Full                  = 1,  ///< Fully enabled Thru (every incoming message is sent back).
	SameChannel           = 2,  ///< Only the messages on the Input Channel will be sent back.
	DifferentChannel      = 3   ///< All the messages but the ones on the Input Channel will be sent back.
};


/*! The midimsg structure contains decoded data of a MIDI message read from the serial port with read() or thru(). \n */
struct midimsg {
	/*! The MIDI channel on which the message was recieved. \n Value goes from 1 to 16. */
	byte channel; 
	/*! The type of the message (see the define section for types reference) */
	kMIDIType type;
	/*! The first data byte.\n Value goes from 0 to 127.\n */
	byte data1;
	/*! The second data byte. If the message is only 2 bytes long, this one is null.\n Value goes from 0 to 127. */
	byte data2;
	/*! System Exclusive dedicated byte array. \n Array length is stocked on 16 bits, in data1 (LSB) and data2 (MSB) */
	byte sysex_array[MIDI_SYSEX_ARRAY_SIZE];
	/*! This boolean indicates if the message is valid or not. There is no channel consideration here, validity means the message respects the MIDI norm. */
	bool valid;
};


/*! \brief The main class for MIDI handling.\n
	See member descriptions to know how to use it,
	or check out the examples supplied with the library.
 */
class MIDI_Class : public QActive {
		
public:
	// Constructor and Destructor
	MIDI_Class();
	~MIDI_Class();	
	
	void Start(uint8_t prio);
	
	void publish_midi_event();
	static void RxCallback(uint8_t id);
	
	bool read();
	bool read(const byte Channel);
	
	// Getters
	kMIDIType getType() const;
	byte getChannel() const;
	byte getData1() const;
	byte getData2() const;
	const byte * getSysExArray() const;
	unsigned int getSysExArrayLength() const;
	bool check() const;
	
	byte getInputChannel() const 
    {
        return mInputChannel;
    }
	
	// Setters
	void setInputChannel(const byte Channel);
	
	/*! \brief Extract an enumerated MIDI type from a status byte.
	 
	 This is a utility static method, used internally, made public so you can handle kMIDITypes more easily.
	 */
	static inline const kMIDIType getTypeFromStatusByte(const byte inStatus) 
    {
		if ((inStatus < 0x80) 
			|| (inStatus == 0xF4) 
			|| (inStatus == 0xF5) 
			|| (inStatus == 0xF9) 
			|| (inStatus == 0xFD)) return InvalidType; // data bytes and undefined.
		if (inStatus < 0xF0) return (kMIDIType)(inStatus & 0xF0);	// Channel message, remove channel nibble.
		else return (kMIDIType)inStatus;
	}
	
protected:
	static QState InitialPseudoState(MIDI_Class * const me, QEvt const * const e);
	static QState Root(MIDI_Class * const me, QEvt const * const e);
	static QState Stopped(MIDI_Class * const me, QEvt const * const e);
	static QState Started(MIDI_Class * const me, QEvt const * const e);

	enum {
		EVT_QUEUE_COUNT = 16
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	uint8_t m_id;
	char const * m_name;
	
	static buffer MIDIBuf;
		
private:
	
	bool input_filter(byte inChannel);
	bool parse(byte inChannel);
	void reset_input_attributes();
	
	// Attributes
	byte			mRunningStatus_RX;
	byte			mInputChannel;
	
	byte			mPendingMessage[MIDI_SYSEX_ARRAY_SIZE];
	unsigned int	mPendingMessageExpectedLenght;
	unsigned int	mPendingMessageIndex;					// Extended to unsigned int for larger sysex payloads.
	
	midimsg			mMessage;
	
};

#endif // LIB_MIDI_H_
