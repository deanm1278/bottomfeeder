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

#include "qpcpp.h"
#include "qp_extras.h"

#include "hsm_id.h"
#include "System.h"
#include "event.h"
#include "Arduino.h"
#include "hsm_id.h"

#include "SynthDefs.h"

Q_DEFINE_THIS_FILE

using namespace FW;

System::System() :
    QActive((QStateHandler)&System::InitialPseudoState), 
    m_id(SYSTEM), m_name("SYSTEM"),
    m_testTimer(this, SYSTEM_TEST_TIMER) {}

QState System::InitialPseudoState(System * const me, QEvt const * const e) {
    (void)e;
    me->m_deferQueue.init(me->m_deferQueueStor, ARRAY_COUNT(me->m_deferQueueStor));

    me->subscribe(SYSTEM_START_REQ);
    me->subscribe(SYSTEM_STOP_REQ);
    me->subscribe(SYSTEM_TEST_TIMER);
    me->subscribe(SYSTEM_DONE);
    me->subscribe(SYSTEM_FAIL);
	
    me->subscribe(USER_LED_TOGGLE_REQ);
	
	me->subscribe(MIDI_UART_START_CFM);
	me->subscribe(MIDI_UART_STOP_CFM);
	
	me->subscribe(FLASH_CONFIG_START_CFM);
	me->subscribe(FLASH_CONFIG_WRITE_DONE);
	me->subscribe(FLASH_CONFIG_START_CFM);
	me->subscribe(FLASH_CONFIG_STOP_CFM);
	
	me->subscribe(FPGA_START_CFM);
	me->subscribe(FPGA_STOP_CFM);
	
	me->subscribe(SYNTH_START_CFM);
	me->subscribe(SYNTH_STOP_CFM);

#ifdef CONFIG_MIDI_USB	
	me->subscribe(MIDI_USB_START_CFM);
	me->subscribe(MIDI_USB_STOP_CFM);
#endif
	
	me->subscribe(SD_START_CFM);
	me->subscribe(SD_STOP_CFM);
      
    return Q_TRAN(&System::Root);
}

QState System::Root(System * const me, QEvt const * const e) {
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
            status = Q_TRAN(&System::Stopped);
            break;
        }
		case SYSTEM_STOP_REQ: {
			LOG_EVENT(e);
			status = Q_TRAN(&System::Stopping);
			break;
		}
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState System::Stopped(System * const me, QEvt const * const e) {
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
        case SYSTEM_STOP_REQ: {
            LOG_EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new SystemStopCfm(req.GetSeq(), ERROR_SUCCESS);
            QF::PUBLISH(evt, me);
            status = Q_HANDLED();
            break;
        }
        case SYSTEM_START_REQ: {
            LOG_EVENT(e);
            status = Q_TRAN(&System::Starting);
            break;
        }
        default: {
            status = Q_SUPER(&System::Root);
            break;
        }
    }
    return status;
}

QState System::Stopping(System * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->m_cfmCount = 0;
			Evt *evt;
			
			evt = new FPGAStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new SDStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
		
			evt = new MIDIUARTStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new FlashConfigStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
#ifdef CONFIG_MIDI_USB	
			evt = new MIDIUSBStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
#endif

			evt = new SynthStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);

			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case FPGA_STOP_CFM:
		case FLASH_CONFIG_STOP_CFM:
		case SD_STOP_CFM:
		case MIDI_USB_STOP_CFM:
		case SYNTH_STOP_CFM:
		case MIDI_UART_STOP_CFM: {
			LOG_EVENT(e);
			me->HandleCfm(ERROR_EVT_CAST(*e), NUM_AO);
			status = Q_HANDLED();
			break;
		}
		case SYSTEM_FAIL:
			Q_ASSERT(0);
			break;
		case SYSTEM_DONE: {
			LOG_EVENT(e);
			Evt *evt = new SystemStartReq(me->m_nextSequence++);
			me->postLIFO(evt);
			status = Q_TRAN(&System::Stopped);
			break;
		}
		default: {
			status = Q_SUPER(&System::Root);
			break;
		}
	}
	return status;
}

QState System::Starting(System * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);

			me->m_cfmCount = 0;
			Evt *evt;
			
			//MUST START FPGA FIRST, BEFORE WE HIJACK THE SPI BUS
			evt = new FPGAStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);

			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			//me->m_stateTimer.disarm();
			status = Q_HANDLED();
			break;
		}
		case FPGA_START_CFM:{
			Evt *evt;
			evt = new SDStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			me->HandleCfm(ERROR_EVT_CAST(*e), NUM_AO);
			status = Q_HANDLED();
			break;
		}
		case SD_START_CFM:{
			Evt *evt;
			evt = new MIDIUARTStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new FlashConfigStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);

#ifdef CONFIG_MIDI_USB			
			evt = new MIDIUSBStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
#endif

			evt = new SynthStartReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			me->HandleCfm(ERROR_EVT_CAST(*e), NUM_AO);
			status = Q_HANDLED();
			break;
		}
		case FLASH_CONFIG_START_CFM:
		case MIDI_USB_START_CFM:
		case SYNTH_START_CFM:
		case MIDI_UART_START_CFM: {
			LOG_EVENT(e);
			me->HandleCfm(ERROR_EVT_CAST(*e), NUM_AO);
			status = Q_HANDLED();
			break;
		}
		case SYSTEM_FAIL:
		case SYSTEM_DONE: {
			LOG_EVENT(e);
			Evt *evt = new SystemStartCfm(me->m_savedInSeq, ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&System::Started);
			break;
		}
		default: {
			status = Q_SUPER(&System::Root);
			break;
		}
	}
	return status;
}

QState System::Started(System * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            LOG_EVENT(e);
            me->m_testTimer.armX(2000, 2000);
			
			//************* OSCILLATORS **********//
			Evt *evt;
			
			evt = new synthSetCCHandler(CC_WT_WAVE, WT_WAVE, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_WT_TRANSPOSE, WT_TRANSPOSE, NULL, 0);
			QF::PUBLISH(evt, me);
			evt = new synthSetCCHandler(CC_WT_TUNE, WT_TUNE, NULL, 0);
			QF::PUBLISH(evt, me);
			evt = new synthSetCCHandler(CC_WT_VOLUME, WT_VOLUME, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_GLIDE_TIME, GLIDE_TIME, NULL, 0);
			QF::PUBLISH(evt, me);
			
			//********** FILTER ****************//
			
			int pwm = CUTOFF;
			evt = new synthSetCCHandler(CC_CUTOFF, CV, &(reinterpret_cast<byte&>(pwm)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			pwm = RESONANCE;
			evt = new synthSetCCHandler(CC_RESONANCE, CV, &(reinterpret_cast<byte&>(pwm)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			//************ ENV ***************//
			
			int param = ATTACK;
			evt = new synthSetCCHandler(CC_ATTACK, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			param = DECAY;
			evt = new synthSetCCHandler(CC_DECAY, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			param = SUSTAIN;
			evt = new synthSetCCHandler(CC_SUSTAIN, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			param = RELEASE;
			evt = new synthSetCCHandler(CC_RELEASE, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			param = CUTOFF_MIX;
			evt = new synthSetCCHandler(CC_ENV_CUTOFF_MIX, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			param = AMP_MIX;
			evt = new synthSetCCHandler(CC_ENV_AMP_MIX, ENV, &(reinterpret_cast<byte&>(param)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			//*********** OTHER CV ***********//
			
			pwm = AMP;
			evt = new synthSetCCHandler(CC_AMP, CV, &(reinterpret_cast<byte&>(pwm)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			pwm = SUB;
			evt = new synthSetCCHandler(CC_SUB, CV, &(reinterpret_cast<byte&>(pwm)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			pwm = NOISE;
			evt = new synthSetCCHandler(CC_NOISE, CV, &(reinterpret_cast<byte&>(pwm)), sizeof(int));
			QF::PUBLISH(evt, me);
			
			//**************** LFO *************//
			
			evt = new synthSetCCHandler(CC_LFO_RATE, LFO_RATE, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_LFO_DEPTH, LFO_DEPTH, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_LFO_TARGET, CC_TYPE_LFO_TARGET, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_LFO_WAVE, LFO_WAVE, NULL, 0);
			QF::PUBLISH(evt, me);
			
			//*********************************//
			
			evt = new synthSetCCHandler(CC_PARA_MODE, PARA_MODE, NULL, 0);
			QF::PUBLISH(evt, me);
			
			//********** PRESETS DO NOT CHANGE!! *************//
			evt = new synthSetCCHandler(CC_PRESET_LOAD_SELECT, PRESET_LOAD_SELECT, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_PRESET_LOAD_ENTER, PRESET_LOAD_ENTER, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_PRESET_STORE_SELECT, PRESET_STORE_SELECT, NULL, 0);
			QF::PUBLISH(evt, me);
			
			evt = new synthSetCCHandler(CC_PRESET_STORE_ENTER, PRESET_STORE_ENTER, NULL, 0);
			QF::PUBLISH(evt, me);

#ifndef PANEL_ATTACHED
			//************* LOAD FIRST PRESET *************//
			evt = new Evt(SYNTH_LOAD_PRESET_REQ);
			QF::PUBLISH(evt, me);
#endif
			
			SerialUSB.println("System started!");
			SerialUSB.print("> ");
			
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            LOG_EVENT(e);
            me->m_testTimer.disarm();
            status = Q_HANDLED();
            break;
        }
        case SYSTEM_TEST_TIMER: {
			//when the timer goes off, this will get hit. Log the event and publish another event
            LOG_EVENT(e);
			
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
			
			if(SerialUSB.available()){
				char c = SerialUSB.read();
				if(c == 'f'){
					SerialUSB.println("Writing firmware... please wait");
					status = Q_TRAN(&System::WriteFirmware);
				}
				
				else if(c == 'w'){
					SerialUSB.println("Writing waveforms... please wait");
					status = Q_TRAN(&System::WriteWaveforms);
				}
				
				else if(c == 'v'){
					Evt *evt;
					evt = new FlashConfigVerifyConfigurationReq(FIRMWARE_PATH);
					QF::PUBLISH(evt, me);
					SerialUSB.println("verifying firmware... please wait");
					status = Q_HANDLED();
				}
				
				else{
					SerialUSB.println("Unrecognized command...");
					SerialUSB.println("f = write FPGA firmware from SD card");
					SerialUSB.println("w = write waveforms from SD card");
					SerialUSB.println("v = verify FPGA configuration");
					SerialUSB.print("> ");
					 status = Q_HANDLED();
				}
			}
            else status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&System::Root);
            break;
        }
    }
    return status;
}

QState System::WriteWaveforms(System * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->m_cfmCount = 0;
			
			Evt *evt = new FPGAStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new MIDIUARTStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new SynthStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new MIDIUSBStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);

			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case MIDI_USB_STOP_CFM:
		case SYNTH_STOP_CFM:
		case FPGA_STOP_CFM:
		case MIDI_UART_STOP_CFM: {
			LOG_EVENT(e);
			me->HandleCfm(ERROR_EVT_CAST(*e), 4);
			status = Q_HANDLED();
			break;
		}
		case SYSTEM_DONE: {
			LOG_EVENT(e);
			Evt *evt = new Evt(FLASH_CONFIG_WRITE_WAVEFORMS_REQ);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		
		case FLASH_CONFIG_WRITE_DONE: {
			LOG_EVENT(e);
			status = Q_TRAN(&System::Stopping);
			break;
		}
		default: {
			status = Q_SUPER(&System::Root);
			break;
		}
	}
	return status;
}

QState System::WriteFirmware(System * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->m_cfmCount = 0;
			
			Evt *evt = new FPGAStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new MIDIUARTStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new SynthStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);
			
			evt = new MIDIUSBStopReq(me->m_nextSequence++);
			// TODO - Save sequence number for comparison.
			QF::PUBLISH(evt, me);

			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case MIDI_USB_STOP_CFM:
		case SYNTH_STOP_CFM:
		case FPGA_STOP_CFM:
		case MIDI_UART_STOP_CFM: {
			LOG_EVENT(e);
			me->HandleCfm(ERROR_EVT_CAST(*e), 4);
			status = Q_HANDLED();
			break;
		}
		case SYSTEM_DONE: {
			LOG_EVENT(e);
			Evt *evt = new FlashConfigWriteConfiguration(FIRMWARE_PATH);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		
		case FLASH_CONFIG_WRITE_DONE: {
			LOG_EVENT(e);
			status = Q_TRAN(&System::Stopping);
			break;
		}
		default: {
			status = Q_SUPER(&System::Root);
			break;
		}
	}
	return status;
}

void System::HandleCfm(ErrorEvt const &e, uint8_t expectedCnt) {
	if (e.GetError() == ERROR_SUCCESS) {
		// TODO - Compare seqeuence number.
		if(++m_cfmCount == expectedCnt) {
			Evt *evt = new Evt(SYSTEM_DONE);
			postLIFO(evt);
		}
		} else {
		Evt *evt = new SystemFail(e.GetError(), e.GetReason());
		postLIFO(evt);
	}
}

