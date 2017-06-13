/*******************************************************************************
 * Copyright (C) Lawrence Lo (https://github.com/galliumstudio). 
 * All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*
* Individual events would all be defined here
*/

#ifndef EVENT_H
#define EVENT_H

#include "qpcpp.h"
#include "qp_extras.h"
#include "buffer.h"
#include "SynthDefs.h"

#define LOG_EVENT(e_)            Log::Event(me->m_name, __FUNCTION__, GetEvtName(e_->sig), e_->sig);

using namespace FW;

enum {
    SYSTEM_START_REQ = QP::Q_USER_SIG,
    SYSTEM_START_CFM,
    SYSTEM_STOP_REQ,
    SYSTEM_STOP_CFM,
    SYSTEM_TEST_TIMER,
    SYSTEM_DONE,
    SYSTEM_FAIL,
    
    USER_LED_TOGGLE_REQ,
	
	MIDI_UART_START_REQ,
	MIDI_UART_START_CFM,
	MIDI_UART_STOP_REQ,
	MIDI_UART_STOP_CFM,
	MIDI_UART_DATA_READY,
	
	FLASH_CONFIG_START_REQ,
	FLASH_CONFIG_START_CFM,
	FLASH_CONFIG_STOP_REQ,
	FLASH_CONFIG_STOP_CFM,
	FLASH_CONFIG_WRITE_CONFIGURATION,
	FLASH_CONFIG_VERIFY_CONFIGURATION_REQ,
	FLASH_CONFIG_WRITE_DONE,
	FLASH_CONFIG_WRITE_WAVEFORMS_REQ,
	FLASH_CONFIG_READ_LFO_REQ,
	FLASH_CONFIG_READ_TO_LISTENER_REQ,
	FLASH_CONFIG_READ_TO_LISTENER_DONE,
	
	SD_START_REQ,
	SD_START_CFM,
	SD_STOP_REQ,
	SD_STOP_CFM,
	SD_READ_FILE_REQ,
	SD_READ_FILE_RESPONSE,
	SD_WRITE_FILE_REQ,
	SD_WRITE_FILE_RESPONSE,
	
	FPGA_START_REQ,
	FPGA_START_CFM,
	FPGA_STOP_REQ,
	FPGA_STOP_CFM,
	FPGA_WRITE_WAVE_FILE,
	FPGA_WRITE_PWM,
	FPGA_WRITE_FS,
	FPGA_WRITE_PARAM_REQ,
	FPGA_NOTIFY_KEY_PRESSED,
	FPGA_SET_PORTAMENTO_REQ,
	FPGA_SET_ENABLE_REQ,
	FPGA_WRITE_VOL_REQ,
	FPGA_SET_WRITE_ENABLE_REQ,
	FPGA_START_TIMER,
	
	SYNTH_START_REQ,
	SYNTH_START_CFM,
	SYNTH_STOP_REQ,
	SYNTH_STOP_CFM,
	SYNTH_UPDATE_TIMER,
	SYNTH_SET_CC_HANDLER,
	SYNTH_SET_LFO_TARGET_REQ,
	SYNTH_SET_LFO_RATE_REQ,
	SYNTH_SET_MODE_PARAPHONIC_REQ,
	SYNTH_SET_MODE_MONOPHONIC_REQ,
	SYNTH_WAVEFORM_TIMER,
	SYNTH_LOAD_PRESET_REQ,
	SYNTH_STORE_PRESET_REQ,
	
	MIDI_USB_START_REQ,
	MIDI_USB_START_CFM,
	MIDI_USB_STOP_REQ,
	MIDI_USB_STOP_CFM,
	MIDI_USB_DATA_READY,
	
	NOTE_ON_REQ,
	NOTE_OFF_REQ,
	CONTROL_CHANGE_REQ,
	PITCH_BEND_REQ,
	AFTER_TOUCH_POLY_REQ,
	AFTER_TOUCH_CHANNEL_REQ,
	PROGRAM_CHANGE_REQ,
	SYSTEM_EXCLUSIVE_REQ,
    
    MAX_PUB_SIG
};

char const * GetEvtName(QP::QSignal sig);

/********** GENERAL MIDI REQS ***********/

class noteOnReq : public Evt {
public:
	noteOnReq(byte channel, byte note, byte velocity) : Evt(NOTE_ON_REQ), channel(channel), note(note), velocity(velocity) {}
	byte getChannel() const { return channel; }
	byte getNote() const { return note; }
	byte getVelocity() const { return velocity; }
		
private:
	byte channel;
	byte note;
	byte velocity;
};

class noteOffReq : public Evt {
	public:
	noteOffReq(byte channel, byte note, byte velocity) : Evt(NOTE_OFF_REQ), channel(channel), note(note), velocity(velocity) {}
	byte getChannel() const { return channel; }
	byte getNote() const { return note; }
	byte getVelocity() const { return velocity; }
	
	private:
	byte channel;
	byte note;
	byte velocity;
};

class controlChangeReq : public Evt {
	public:
	controlChangeReq(byte channel, byte cc, byte value) : Evt(CONTROL_CHANGE_REQ), channel(channel), cc(cc), value(value) {}
	byte getChannel() const { return channel; }
	byte getCC() const { return cc; }
	byte getValue() const { return value; }
	
	private:
	byte channel;
	byte cc;
	byte value;
};

class pitchBendReq : public Evt {
	public:
	pitchBendReq(byte channel, int value) : Evt(PITCH_BEND_REQ), channel(channel), value(value) {}
	byte getChannel() const { return channel; }
	int getValue() const { return value; }
	
	private:
	byte channel;
	int value;
};

/********** END GENERAL MIDI REQS ***********/

class SystemStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 600
    };
    SystemStartReq(uint16_t seq) :
        Evt(SYSTEM_START_REQ, seq) {}
};

class SystemStartCfm : public ErrorEvt {
public:
    SystemStartCfm(uint16_t seq, Error error, Reason reason = 0) :
        ErrorEvt(SYSTEM_START_CFM, seq, error, reason) {}
};

class SystemStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 600
    };
    SystemStopReq(uint16_t seq) :
        Evt(SYSTEM_STOP_REQ, seq) {}
};

class SystemStopCfm : public ErrorEvt {
public:
    SystemStopCfm(uint16_t seq, Error error, Reason reason = 0) :
        ErrorEvt(SYSTEM_STOP_CFM, seq, error, reason) {}
};

class SystemFail : public ErrorEvt {
public:
    SystemFail(Error error, Reason reason) :
        ErrorEvt(SYSTEM_FAIL, 0, error, reason) {}
};

class UserLedToggleReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    UserLedToggleReq(uint16_t seq) :
        Evt(USER_LED_TOGGLE_REQ, seq) {}
};

/************ SYNTH *****************/

class SynthStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	SynthStartReq(uint16_t seq) :
	Evt(SYNTH_START_REQ, seq) {}
};

class SynthStartCfm : public ErrorEvt {
	public:
	SynthStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(SYNTH_START_CFM, seq, error, reason) {}
};

class SynthStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	SynthStopReq(uint16_t seq) :
	Evt(SYNTH_STOP_REQ, seq) {}
};

class SynthStopCfm : public ErrorEvt {
	public:
	SynthStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(SYNTH_STOP_CFM, seq, error, reason) {}
};

class synthSetCCHandler : public Evt {
	public:
	synthSetCCHandler(byte cc, ccType_t type, byte *args, int count) : Evt(SYNTH_SET_CC_HANDLER), 
	cc(cc), type(type), count(count) {
		memcpy(this->args, args, count);
	}
	byte getCC() const { return cc; }
	ccType_t getType() const { return type; }
	byte *getArgs() const { return (byte *)args; }
	int getCount() const { return count; }
	
	private:
	byte cc;
	ccType_t type;
	byte args[CC_ARGS_MAX_LENGTH];
	int count; 
};

class synthSetLFOTargetReq : public Evt {
	public:
	
	synthSetLFOTargetReq(uint8_t num, uint8_t type, uint8_t id) :
	Evt(SYNTH_SET_LFO_TARGET_REQ), num(num), type(type), id(id) {}
	
	uint8_t getNum() const { return num; }
	uint8_t getType() const { return type; }
	uint8_t getId() const { return id; }
		
	private:
	uint8_t num, type, id;
};

class synthSetLFORateReq : public Evt {
	public:
	
	synthSetLFORateReq(uint8_t num, uint16_t rate) :
	Evt(SYNTH_SET_LFO_RATE_REQ), num(num), rate(rate) {}
	
	uint8_t getNum() const { return num; }
	uint16_t getRate() const { return rate; }
	
	private:
	uint8_t num;
	uint16_t rate;
};

/*********** END SYNTH **************/

/************ MIDIUSB *****************/

class MIDIUSBStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	MIDIUSBStartReq(uint16_t seq) :
	Evt(MIDI_USB_START_REQ, seq) {}
};

class MIDIUSBStartCfm : public ErrorEvt {
	public:
	MIDIUSBStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(MIDI_USB_START_CFM, seq, error, reason) {}
};

class MIDIUSBStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	MIDIUSBStopReq(uint16_t seq) :
	Evt(MIDI_USB_STOP_REQ, seq) {}
};

class MIDIUSBStopCfm : public ErrorEvt {
	public:
	MIDIUSBStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(MIDI_USB_STOP_CFM, seq, error, reason) {}
};

/*********** END MIDIUSB **************/

/************** SD CARD ***************/

class SDStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	SDStartReq(uint16_t seq) :
	Evt(SD_START_REQ, seq) {}
};

class SDStartCfm : public ErrorEvt {
	public:
	SDStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(SD_START_CFM, seq, error, reason) {}
};

class SDStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	SDStopReq(uint16_t seq) :
	Evt(SD_STOP_REQ, seq) {}
};

class SDStopCfm : public ErrorEvt {
	public:
	SDStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(SD_STOP_CFM, seq, error, reason) {}
};

class SDReadFileReq : public Evt {
public:
   
	SDReadFileReq(uint16_t seq, char *fn, uint32_t pos, uint16_t numBytes, buffer *buf) :
	Evt(SD_READ_FILE_REQ, seq), pos(pos), numBytes(numBytes), buf(buf) {
		strcpy(filename, fn);
	}
			
	char *getFilename() const { return (char *)filename; }
	uint32_t getPos() const { return pos; }
	uint16_t getNumBytes() const { return numBytes; }
	buffer *getBuf() const { return buf; }
private:
    char filename[50];
	uint32_t pos;
	uint16_t numBytes;
	buffer *buf;
};

class SDReadFileResponse : public Evt {
	public:
	
	SDReadFileResponse(uint16_t seq, char *fn, bool eof, uint32_t exitPos, uint16_t bytesRead, buffer *buf, bool error) :
	Evt(SD_READ_FILE_RESPONSE, seq), eof(eof), exitPos(exitPos), bytesRead(bytesRead), buf(buf), error(error) {
		strcpy(filename, fn);
	}
	
	char *getFilename() const { return (char *)filename; }
	uint32_t getExitPos() const { return exitPos; }
	uint16_t getBytesRead() const { return bytesRead; }
	bool getEof() const { return eof; }
	buffer *getBuf() const { return buf; }
	bool getError() const { return error; }
private:
	char filename[50];
	bool eof;
	uint32_t exitPos;
	uint16_t bytesRead;
	buffer *buf;
	bool error;
};

class SDWriteFileReq : public Evt {
	public:
	
	SDWriteFileReq(uint16_t seq, char *fn, uint32_t pos, uint16_t numBytes, buffer *buf) :
	Evt(SD_WRITE_FILE_REQ, seq), pos(pos), numBytes(numBytes), buf(buf) {
		strcpy(filename, fn);
	}
	
	char *getFilename() const { return (char *)filename; }
	uint32_t getPos() const { return pos; }
	uint16_t getNumBytes() const { return numBytes; }
	buffer *getBuf() const { return buf; }
	private:
	char filename[50];
	uint32_t pos;
	uint16_t numBytes;
	buffer *buf;
};

class SDWriteFileResponse : public Evt {
	public:
	
	SDWriteFileResponse(uint16_t seq, char *fn, uint32_t exitPos, buffer *buf, bool error) :
	Evt(SD_WRITE_FILE_RESPONSE, seq), exitPos(exitPos), buf(buf), error(error) {
		strcpy(filename, fn);
	}
	
	char *getFilename() const { return (char *)filename; }
	uint32_t getExitPos() const { return exitPos; }
	buffer *getBuf() const { return buf; }
	bool getError() const { return error; }
	private:
	char filename[50];
	uint32_t exitPos;
	buffer *buf;
	bool error;
};

/************ END SD CARD **********/

/************ FPGA *****************/

class FPGAStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	FPGAStartReq(uint16_t seq) :
	Evt(FPGA_START_REQ, seq) {}
};

class FPGAStartCfm : public ErrorEvt {
	public:
	FPGAStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(FPGA_START_CFM, seq, error, reason) {}
};

class FPGAStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	FPGAStopReq(uint16_t seq) :
	Evt(FPGA_STOP_REQ, seq) {}
};

class FPGAStopCfm : public ErrorEvt {
	public:
	FPGAStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(FPGA_STOP_CFM, seq, error, reason) {}
};

class FPGAWriteWaveFile : public Evt {
	public:
	
	FPGAWriteWaveFile(uint8_t num, uint8_t channel) :
	Evt(FPGA_WRITE_WAVE_FILE), num(num), channel(channel) {}
	
	uint8_t getnum() const { return num; }
	uint8_t getChannel() const { return channel; }
	private:
	uint8_t num;
	uint8_t channel;
};

class FPGAWritePWM : public Evt {
	public:
	
	FPGAWritePWM(uint8_t pwm_number, uint16_t value) :
	Evt(FPGA_WRITE_PWM), pwm_number(pwm_number), value(value) {}
	
	const uint8_t getPwmNumber() const { return pwm_number; }
	const uint16_t getValue() const { return value; }
	private:
	uint8_t pwm_number;
	uint16_t value;
};

class FPGAWriteFS : public Evt {
	public:
	
	FPGAWriteFS(uint8_t channel, uint16_t fs) :
	Evt(FPGA_WRITE_FS), channel(channel), fs(fs) {}
	
	const uint8_t getChannel() const { return channel; }
	const uint16_t getFs() const { return fs; }
	private:
	uint8_t channel;
	uint16_t fs;
};

class FPGAWriteParamReq : public Evt {
	public:
	
	FPGAWriteParamReq(uint8_t param, uint32_t value) :
	Evt(FPGA_WRITE_PARAM_REQ), param(param), value(value) {}
	
	const uint8_t getParam() const { return param; }
	const uint32_t getValue() const { return value; }
	private:
	uint8_t param;
	uint32_t value;
};

class FPGANotifyKeyPressed : public Evt {
	public:
	
	FPGANotifyKeyPressed(bool pressed) :
	Evt(FPGA_NOTIFY_KEY_PRESSED), pressed(pressed){}
	
	const bool getPressed() const { return pressed; }
	private:
	bool pressed;
};

class FPGASetPortamentoReq : public Evt {
	public:
	
	FPGASetPortamentoReq(uint16_t prescale) :
	Evt(FPGA_SET_PORTAMENTO_REQ), prescale(prescale){}
	
	const uint16_t getPrescale() const { return prescale; }
	private:
	uint16_t prescale;
};

class FPGASetEnableReq: public Evt{
public:
	FPGASetEnableReq(uint16_t en):
	Evt(FPGA_SET_ENABLE_REQ), en(en) {}
		
	const uint16_t getEnable() const { return en; }
private:
	uint16_t en;
};

class FPGAWriteVolReq : public Evt {
	public:
	
	FPGAWriteVolReq(uint8_t channel, uint16_t vol) :
	Evt(FPGA_WRITE_VOL_REQ), channel(channel), vol(vol) {}
	
	const uint8_t getChannel() const { return channel; }
	const uint16_t getVol() const { return vol; }
	private:
	uint8_t channel;
	uint16_t vol;
};

/*********** END FPGA **************/

/************** FLASH CONFIG ***************/

class FlashConfigStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	FlashConfigStartReq(uint16_t seq) :
	Evt(FLASH_CONFIG_START_REQ, seq) {}
};

class FlashConfigStartCfm : public ErrorEvt {
	public:
	FlashConfigStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(FLASH_CONFIG_START_CFM, seq, error, reason) {}
};

class FlashConfigStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	FlashConfigStopReq(uint16_t seq) :
	Evt(FLASH_CONFIG_STOP_REQ, seq) {}
};

class FlashConfigStopCfm : public ErrorEvt {
	public:
	FlashConfigStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(FLASH_CONFIG_STOP_CFM, seq, error, reason) {}
};

class FlashConfigWriteConfiguration : public Evt {
	public:
	
	FlashConfigWriteConfiguration(const char *fw_path) :
	Evt(FLASH_CONFIG_WRITE_CONFIGURATION), fw_path(fw_path) {}
	
	const char *getFwPath() const { return fw_path; }
	private:
	const char *fw_path;
};

class FlashConfigVerifyConfigurationReq : public Evt {
	public:
	
	FlashConfigVerifyConfigurationReq(const char *fw_path) :
	Evt(FLASH_CONFIG_VERIFY_CONFIGURATION_REQ), fw_path(fw_path) {}
	
	const char *getFwPath() const { return fw_path; }
	private:
	const char *fw_path;
};

class FlashConfigReadLFOReq : public Evt {
	public:
	
	FlashConfigReadLFOReq(uint8_t num, signed short *writeTo) :
	Evt(FLASH_CONFIG_READ_LFO_REQ), num(num), writeTo(writeTo) { }
	
	uint8_t getNum() const { return num; }
	signed short *getWriteTo() const { return writeTo; }
	private:
	uint8_t num;
	signed short *writeTo;
};

class FlashConfigReadToListenerReq : public Evt {
	public:
	
	FlashConfigReadToListenerReq(uint8_t num) :
	Evt(FLASH_CONFIG_READ_TO_LISTENER_REQ), num(num) {}
	
	uint8_t getNum() const { return num; }
	private:
	uint8_t num;
};


/************ END FLASH CONFIG **********/

/************** MIDI UART ***************/

class MIDIUARTStartReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	MIDIUARTStartReq(uint16_t seq) :
	Evt(MIDI_UART_START_REQ, seq) {}
};

class MIDIUARTStartCfm : public ErrorEvt {
	public:
	MIDIUARTStartCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(MIDI_UART_START_CFM, seq, error, reason) {}
};

class MIDIUARTStopReq : public Evt {
	public:
	enum {
		TIMEOUT_MS = 100
	};
	MIDIUARTStopReq(uint16_t seq) :
	Evt(MIDI_UART_STOP_REQ, seq) {}
};

class MIDIUARTStopCfm : public ErrorEvt {
	public:
	MIDIUARTStopCfm(uint16_t seq, Error error, Reason reason = 0) :
	ErrorEvt(MIDI_UART_STOP_CFM, seq, error, reason) {}
};

/************** END MIDI UART ***************/

#endif