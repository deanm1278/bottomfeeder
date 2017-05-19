/*
** Copyright (c) 2015, Gary Grewal
** Modified 2016, Dean Miller. Some stuff copied from code by Paul Stoffregen
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "MIDIUSB2.h"

#define MIDI_AC_INTERFACE 	pluggedInterface	// MIDI AC Interface
#define MIDI_INTERFACE 		pluggedInterface+1
#define MIDI_FIRST_ENDPOINT pluggedEndpoint
#define MIDI_ENDPOINT_OUT	pluggedEndpoint
#define MIDI_ENDPOINT_IN	pluggedEndpoint+1

#define MIDI_RX MIDI_ENDPOINT_OUT
#define MIDI_TX MIDI_ENDPOINT_IN

int MIDI_::getInterface(uint8_t* interfaceNum)
{
	interfaceNum[0] += 2;	// uses 2 interfaces
	MIDIDescriptor _midiInterface =
	{
		D_IAD(MIDI_AC_INTERFACE, 2, MIDI_AUDIO, MIDI_AUDIO_CONTROL, 0),
		D_INTERFACE(MIDI_AC_INTERFACE,0,MIDI_AUDIO,MIDI_AUDIO_CONTROL,0),
		D_AC_INTERFACE(0x1, MIDI_INTERFACE),
		D_INTERFACE(MIDI_INTERFACE,2, MIDI_AUDIO,MIDI_STREAMING,0),
		D_AS_INTERFACE,
		D_MIDI_INJACK(MIDI_JACK_EMD, 0x1),
		D_MIDI_INJACK(MIDI_JACK_EXT, 0x2),
		D_MIDI_OUTJACK(MIDI_JACK_EMD, 0x3, 1, 2, 1),
		D_MIDI_OUTJACK(MIDI_JACK_EXT, 0x4, 1, 1, 1),
		D_MIDI_JACK_EP(USB_ENDPOINT_OUT(MIDI_ENDPOINT_OUT),USB_ENDPOINT_TYPE_BULK,MIDI_BUFFER_SIZE),
		D_MIDI_AC_JACK_EP(1, 1),
		D_MIDI_JACK_EP(USB_ENDPOINT_IN(MIDI_ENDPOINT_IN),USB_ENDPOINT_TYPE_BULK,MIDI_BUFFER_SIZE),
		D_MIDI_AC_JACK_EP (1, 3)
	};
	return USB_SendControl(0, &_midiInterface, sizeof(_midiInterface));
}

bool MIDI_::setup(USBSetup& setup __attribute__((unused)))
{
	//Support requests here if needed. Typically these are optional
	return false;
}

int MIDI_::getDescriptor(USBSetup& setup __attribute__((unused)))
{
	return 0;
}

uint8_t MIDI_::getShortName(char* name)
{
	memcpy(name, "MIDI", 4);
	return 4;
}

void MIDI_::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
	send_raw(0x08, 0x80 | ((channel - 1) & 0x0F), note & 0x7F, velocity & 0x7F);
}
void MIDI_::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
	send_raw(0x09, 0x90 | ((channel - 1) & 0x0F), note & 0x7F, velocity & 0x7F);
}
void MIDI_::sendPolyPressure(uint8_t note, uint8_t pressure, uint8_t channel)
{
	send_raw(0x0A, 0xA0 | ((channel - 1) & 0x0F), note & 0x7F, pressure & 0x7F);
}
void MIDI_::sendControlChange(uint8_t control, uint8_t value, uint8_t channel)
{
	send_raw(0x0B, 0xB0 | ((channel - 1) & 0x0F), control & 0x7F, value & 0x7F);
}
void MIDI_::sendProgramChange(uint8_t program, uint8_t channel)
{
	send_raw(0x0C, 0xC0 | ((channel - 1) & 0x0F), program & 0x7F, 0);
}
void MIDI_::sendAfterTouch(uint8_t pressure, uint8_t channel)
{
	send_raw(0x0D, 0xD0 | ((channel - 1) & 0x0F), pressure & 0x7F, 0);
}
void MIDI_::sendPitchBend(uint16_t value, uint8_t channel)
{
	send_raw(0x0E, 0xE0 | ((channel - 1) & 0x0F), value & 0x7F, (value >> 7) & 0x7F);
}
void MIDI_::send_raw(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    uint8_t data[4];
    data[0] = b0;
    data[1] = b1;
    data[2] = b2;
    data[3] = b3;
    write(data, 4);
    flush();
}

bool MIDI_::available(){
	uint8_t c;
	if (!USB_Available(MIDI_RX)) {
		#if defined(ARDUINO_ARCH_SAM)
		udd_ack_fifocon(MIDI_RX);
		#endif
		//break;
	}
	
	//TODO: lets actually put these into a buffer
	c = USB_Recv(MIDI_RX, &event, sizeof (event));
	if (c < 4) return false;
	else return true;
}

bool MIDI_::read(uint8_t *newevt, uint8_t channel) {
    uint8_t c;
    uint8_t b0, b1, b2, b3, type1, type2;

    b0 = newevt[0];
    b1 = newevt[1];
    b2 = newevt[2];
    b3 = newevt[3];

    type1 = b0 & 0x0F;
    type2 = b1 & 0xF0;
    
    c = (b1 & 0x0F) + 1;
    if (type1 >= 0x08 && type1 <= 0x0E) {
        if (channel && channel != c) {
            // ignore other channels when user wants single channel read
            return false;
        }
        if (type1 == 0x08 && type2 == 0x80) {
            msg_type = NOTE_OFF; // Note off
            goto return_message;
        }
        if (type1 == 0x09 && type2 == 0x90) {
            if (b3) {
                msg_type = NOTE_ON; // Note on
            } else {
                msg_type = 0; // Note off
            }
            goto return_message;
        }
        if (type1 == 0x0A && type2 == 0xA0) {
            msg_type = POLY_PRESSURE; // Poly Pressure
            goto return_message;
        }
        if (type1 == 0x0B && type2 == 0xB0) {
            msg_type = CONTROL_CHANGE; // Control Change
            goto return_message;
        }
        if (type1 == 0x0C && type2 == 0xC0) {
            msg_type = PROGRAM_CHANGE; // Program Change
            goto return_message;
        }
        if (type1 == 0x0D && type2 == 0xD0) {
            msg_type = 5; // After Touch
            goto return_message;
        }
        if (type1 == 0x0E && type2 == 0xE0) {
            msg_type = PITCH_BEND; // Pitch Bend
            goto return_message;
        }
        return false;
return_message:
        // only update these when returning true for a parsed message
        // all other return cases will preserve these user-visible values
        msg_channel = c;
        msg_data1 = b2;
        msg_data2 = b3;
        return true;
    }
    if (type1 == 0x04) {
        read_sysex_byte(b1);
        read_sysex_byte(b2);
        read_sysex_byte(b3);
        return false;
    }
    if (type1 >= 0x05 && type1 <= 0x07) {
        
        read_sysex_byte(b1);
        if (type1 >= 0x06) read_sysex_byte(b2);
        if (type1 == 0x07) read_sysex_byte(b3);
        //if (handleSystemExclusive) (*handleSystemExclusive)(msg_sysex, msg_sysex_len);
        msg_data1 = msg_sysex_len;
        msg_sysex_len = 0;
        msg_type = SYSEX;
        return true;
    }
    if (type1 == 0x0F) {
        // TODO: does this need to be a full MIDI parser?
        // What software actually uses this message type in practice?
        if (msg_sysex_len > 0) {
            // From David Sorlien, dsorlien at gmail.com, http://axe4live.wordpress.com
            // OSX sometimes uses Single Byte Unparsed to
            // send bytes in the middle of a SYSEX message.
            read_sysex_byte(b1);
        } else {
            // From Sebastian Tomczak, seb.tomczak at gmail.com
            // http://little-scale.blogspot.com/2011/08/usb-midi-game-boy-sync-for-16.html
            msg_type = REAL_TIME_SYSTEM;
            //if (handleRealTimeSystem) (*handleRealTimeSystem)(b1);
            goto return_message;
        }
    }
    return false;
}

void MIDI_::read_sysex_byte(uint8_t b)
{
	if (msg_sysex_len < USB_MIDI_SYSEX_MAX) {
		msg_sysex[msg_sysex_len++] = b;
	}
}

void MIDI_::flush(void)
{
	USB_Flush(MIDI_TX);
}

size_t MIDI_::write(const uint8_t *buffer, size_t size)
{
	/* only try to send bytes if the high-level MIDI connection itself
	 is open (not just the pipe) - the OS should set lineState when the port
	 is opened and clear lineState when the port is closed.
	 bytes sent before the user opens the connection or after
	 the connection is closed are lost - just like with a UART. */

	// TODO - ZE - check behavior on different OSes and test what happens if an
	// open connection isn't broken cleanly (cable is yanked out, host dies
	// or locks up, or host virtual serial port hangs)

	// first, check the TX buffer to see if it's ready for writing.
	// USB_Send() may block if there's no one listening on the other end.
	// in that case, we don't want to block waiting for someone to connect,
	// because that would freeze the whole sketch
	// instead, we'll just drop the packets and hope the caller figures it out.
	if (is_write_enabled(MIDI_TX))
	{

		int r = USB_Send(MIDI_TX, buffer, size);

		if (r > 0)
		{
			return r;
		} else
		{
			return 0;
		}
	}
	return 0;
}

void MIDI_::sendMIDI(midiEventPacket_t event)
{
	uint8_t data[4];
	data[0] = event.header;
	data[1] = event.byte1;
	data[2] = event.byte2;
	data[3] = event.byte3;
	write(data, 4);
}

MIDI_::MIDI_(void) : PluggableUSBModule(2, 2, epType)
{
	epType[0] = EP_TYPE_BULK_OUT_MIDI;	// MIDI_ENDPOINT_OUT
	epType[1] = EP_TYPE_BULK_IN_MIDI;		// MIDI_ENDPOINT_IN
	PluggableUSB().plug(this);
}
