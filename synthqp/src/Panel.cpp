#include "Panel.h"
#include "Arduino.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"

#include "hsm_id.h"

Q_DEFINE_THIS_FILE

enum {
	OSC_0_WAVEFORM = 0,
	OSC_0_COARSE_TUNE,
	OSC_0_VOLUME,
	OSC_0_FINE_TUNE,
	
	OSC_1_VOLUME,
	OSC_1_WAVEFORM,
	OSC_1_FINE_TUNE,
	OSC_1_COARSE_TUNE,
	
	OSC_2_VOLUME,
	OSC_2_WAVEFORM,
	OSC_2_COARSE_TUNE,
	OSC_2_FINE_TUNE,
	
	VOLUME_SUB,
	GLIDE_RATE, //VOLUME_NOISE,
	
	FILTER_CUTOFF,
	FILTER_ENV_MIX,
	
	LFO_0_TARGET,
	LFO_0_WAVEFORM,
	
	FILTER_RESONANCE,
	
	LFO_0_RATE,
	LFO_0_DEPTH,
	
	LFO_1_TARGET,
	LFO_1_DEPTH,
	LFO_1_RATE,
	LFO_1_WAVEFORM,
	
	VCA_VOLUME,
	VCA_ENV_MIX,
	
	ENV_RELEASE,
	ENV_SUSTAIN,
	ENV_DECAY,
	ENV_ATTACK,
};

mcp3008 Panel::adcs[PANEL_NUM_ADC] = { mcp3008(A5), mcp3008(4), mcp3008(1), mcp3008(8) };

Panel::Panel() :
QActive((QStateHandler)&Panel::InitialPseudoState),
m_id(PANEL), m_name("Panel") {}

Panel::~Panel() {}
	
uint8_t tick_div = 0;

void Panel::Start(uint8_t prio) {
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState Panel::InitialPseudoState(Panel * const me, QEvt const * const e) {
	(void)e;
	
	me->subscribe(PANEL_READ_CONTROLS);
	
	return Q_TRAN(&Panel::Root);
}

QState Panel::Root(Panel * const me, QEvt const * const e) {
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
			status = Q_TRAN(&Panel::Started);
			break;
		}
		default: {
			status = Q_SUPER(&QHsm::top);
			break;
		}
	}
	return status;
}

QState Panel::Started(Panel * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->m_paraButtonState = false;
			me->m_paraMode = false;
			pinMode(A1, OUTPUT);
			digitalWrite(A1, HIGH);
			pinMode(6, INPUT_PULLUP);
			
			for(int i=0; i<PANEL_NUM_ADC; i++){
				Panel::adcs[i].begin();
			}
			
			//set initial values
			uint8_t pos = 0;
			for(int i=0; i<PANEL_NUM_ADC; i++){
				QF_CRIT_STAT_TYPE crit;
				QF_CRIT_ENTRY(crit);
				Panel::adcs[i].readAllChannels(me->m_previousValues + pos);
				QF_CRIT_EXIT(crit);
				pos += 8;
			}
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case PANEL_READ_CONTROLS: {
			//LOG_EVENT(e);
			
			uint16_t vals[32];
			uint8_t pos = 0;
			
			for(int i=0; i<PANEL_NUM_ADC; i++){
				QF_CRIT_STAT_TYPE crit;
				QF_CRIT_ENTRY(crit);
				Panel::adcs[i].readAllChannels(vals + pos);
				QF_CRIT_EXIT(crit);
				pos += 8;
			}
			
			for(int i=0; i<PANEL_NUM_ADC_CHANNELS; i++){
				uint16_t val = vals[i];
				
				if(val > me->m_previousValues[i] + PANEL_CHANGE_THRESHOLD ||
					val < me->m_previousValues[i] - PANEL_CHANGE_THRESHOLD || 
					(val == 0 && me->m_previousValues[i] != 0) || (val == 1023 && me->m_previousValues[i] != 1023)){
						if(i == LFO_0_TARGET || i == LFO_1_TARGET){
							if(val > me->m_previousValues[i] + 25 ||
								val < me->m_previousValues[i] - 25){
									me->m_previousValues[i] = val;
									me->sendChange(i);
								}
						}
						else{
							me->m_previousValues[i] = val;
							me->sendChange(i);
						}
					}
			}
			
			bool btn_state = digitalRead(6);
			if(!btn_state && me->m_paraButtonState){
				me->m_paraMode = !me->m_paraMode;
				uint8_t val = (me->m_paraMode ? 127 : 0);
				Evt *evt = new controlChangeReq(1, CC_PARA_MODE, val);
				QF::PUBLISH(evt, this);
				digitalWrite(A1, !me->m_paraMode);
			}
			me->m_paraButtonState = btn_state;
						
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&Panel::Root);
			break;
		}
	}
	return status;
}

void Panel::timerCallback(){
	tick_div++;
	if(tick_div % 4 == 0){
		Evt *evt = new Evt(PANEL_READ_CONTROLS);
		QF::PUBLISH(evt, 0);
		tick_div = 0;
	}
}

void Panel::sendChange(uint8_t ix){
	
	byte channel = 1;
	byte cc;
	byte val = map(m_previousValues[ix], 0, 1023, 0, 127);
	switch(ix){
		case OSC_0_WAVEFORM:
			channel = 1;
			cc = CC_WT_WAVE;
			break;
		case OSC_0_COARSE_TUNE:
			channel = 1;
			cc = CC_WT_TRANSPOSE;
			break;
		case OSC_0_VOLUME: 
			channel = 1;
			cc = CC_WT_VOLUME;
			break;
		case OSC_0_FINE_TUNE:
			channel = 1;
			cc = CC_WT_TUNE;
			break;
		case OSC_1_WAVEFORM: 
			channel = 2;
			cc = CC_WT_WAVE;
			break;
		case OSC_1_COARSE_TUNE:
			channel = 2;
			cc = CC_WT_TRANSPOSE;
			break;
		case OSC_1_VOLUME:
			channel = 2;
			cc = CC_WT_VOLUME;
			break;
		case OSC_1_FINE_TUNE:
			channel = 2;
			cc = CC_WT_TUNE;
			break;
		case OSC_2_VOLUME:
			channel = 3;
			cc = CC_WT_VOLUME;
			break;
		case OSC_2_WAVEFORM:
			channel = 3;
			cc = CC_WT_WAVE;
			break;
		case OSC_2_COARSE_TUNE:
			channel = 3;
			cc = CC_WT_TRANSPOSE;
			break;
		case OSC_2_FINE_TUNE:
			channel = 3;
			cc = CC_WT_TUNE;
			break;
		case VOLUME_SUB:
			cc = CC_SUB;
			break;
			/*
		case VOLUME_NOISE:
			cc = CC_NOISE;
			break;
			*/
		case GLIDE_RATE:
			cc = CC_GLIDE_TIME;
			break;
		case FILTER_CUTOFF:
			cc = CC_CUTOFF;
			break;
		case FILTER_ENV_MIX:
			cc = CC_ENV_CUTOFF_MIX;
			break;
		case LFO_0_TARGET:
			channel = 1;
			val = map(m_previousValues[ix], 0, 1023, 8, 0);
			val = (val == 0 ? 1 : val);
			cc = CC_LFO_TARGET;
			break;
		case LFO_0_WAVEFORM:
			channel = 1;
			cc = CC_LFO_WAVE;
			break;
		case FILTER_RESONANCE:
			cc = CC_RESONANCE;
			break;
		case LFO_0_RATE:
			channel = 1;
			cc = CC_LFO_RATE;
			break;
		case LFO_0_DEPTH:
			channel = 1;
			cc = CC_LFO_DEPTH;
			break;
		case LFO_1_TARGET:
			channel = 2;
			val = map(m_previousValues[ix], 0, 1023, 8, 0);
			val = (val == 0 ? 1 : val);
			cc = CC_LFO_TARGET;
			break;
		case LFO_1_DEPTH:
			channel = 2;
			cc = CC_LFO_DEPTH;
			break;
		case LFO_1_RATE:
			channel = 2;
			cc = CC_LFO_RATE;
			break;
		case LFO_1_WAVEFORM:
			channel = 2;
			cc = CC_LFO_WAVE;
			break;
		case VCA_VOLUME:
			cc = CC_AMP;
			break;
		case VCA_ENV_MIX:
			cc = CC_ENV_AMP_MIX;
			break;
		case ENV_RELEASE:
			cc = CC_RELEASE;
			break;
		case ENV_SUSTAIN:
			cc = CC_SUSTAIN;
			break;
		case ENV_DECAY:
			cc = CC_DECAY;
			break;
		case ENV_ATTACK:
			cc = CC_ATTACK;
			break;
		default:
			Q_ASSERT(0);
			break;
	}
	
	Evt *evt = new controlChangeReq(channel, cc, val);
	QF::PUBLISH(evt, this);
}