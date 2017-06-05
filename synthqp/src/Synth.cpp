#include "Arduino.h"
#include "Synth.h"

#include "qpcpp.h"
#include "qp_extras.h"
#include "event.h"
#include "noteDefs.h"
#include "waveDefs.h"
#include "envDefs.h"

#include "bsp.h"

Q_DEFINE_THIS_FILE

#define SD_READ_MAX 256
static byte sdbuf[SD_READ_MAX];

wavetable Synth::waves[] = {wavetable(), wavetable(), wavetable()};
cv Synth::cvs[] = {cv(600, 4095), cv(), cv(), cv(), cv()};
LFO Synth::lfos[] = {LFO(TC4), LFO(TC5)};
struct note *Synth::notebuf[] = {NULL, NULL, NULL, NULL, NULL, NULL};
struct CC_LOG Synth::cc_log[MAX_CC];

Synth::Synth() :
QActive((QStateHandler)&Synth::InitialPseudoState),
m_id(SYNTH), m_name("Synth"), adsr(), SDBuffer(sdbuf, SD_READ_MAX) {}

Synth::~Synth() {}

void Synth::Start(uint8_t prio) {
	
	QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
};

QState Synth::InitialPseudoState(Synth * const me, QEvt const * const e) {
	(void)e;
	
	me->m_deferQueue.init(me->m_deferQueueStor, ARRAY_COUNT(me->m_deferQueueStor));
	
	me->subscribe(SYNTH_START_REQ);
	me->subscribe(SYNTH_STOP_REQ);
	
	me->subscribe(NOTE_ON_REQ);
	me->subscribe(NOTE_OFF_REQ);
	me->subscribe(CONTROL_CHANGE_REQ);
	me->subscribe(PITCH_BEND_REQ);
	
	me->subscribe(SYNTH_UPDATE_TIMER);
	me->subscribe(SYNTH_SET_CC_HANDLER);
	
	me->subscribe(SYNTH_SET_LFO_TARGET_REQ);
	
	me->subscribe(SYNTH_SET_MODE_PARAPHONIC_REQ);
	me->subscribe(SYNTH_SET_MODE_MONOPHONIC_REQ);
	
	me->subscribe(SYNTH_LOAD_PRESET_REQ);
	me->subscribe(SYNTH_STORE_PRESET_REQ);
	
	me->subscribe(SD_READ_FILE_RESPONSE);
	me->subscribe(SD_WRITE_FILE_RESPONSE);
	
	return Q_TRAN(&Synth::Root);
}

QState Synth::Root(Synth * const me, QEvt const * const e) {
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
			status = Q_TRAN(&Synth::Stopped);
			break;
		}
		case SYNTH_START_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SynthStartCfm(req.GetSeq(), ERROR_STATE);
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

QState Synth::Stopped(Synth * const me, QEvt const * const e) {
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
		case SYNTH_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SynthStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_HANDLED();
			break;
		}
		case SYNTH_START_REQ: {
			LOG_EVENT(e);
			
			//set the update timer
			setTimerFreq(SYNTH_TC, SYNTH_UPDATE_FREQ);
			
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SynthStartCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&Synth::Started);
			break;
		}
		default: {
			status = Q_SUPER(&Synth::Root);
			break;
		}
	}
	return status;
}

QState Synth::Started(Synth * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->startTimer();
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			me->stopTimer();
			status = Q_HANDLED();
			break;
		}
		case Q_INIT_SIG:{
			LOG_EVENT(e);
			if(me->cc_log[PARA_MODE_CC].values[0] > 63) status = Q_TRAN(&Synth::Paraphonic);
			else status = Q_TRAN(&Synth::Monophonic);
			break;
		}
		case SYNTH_STOP_REQ: {
			LOG_EVENT(e);
			Evt const &req = EVT_CAST(*e);
			Evt *evt = new SynthStopCfm(req.GetSeq(), ERROR_SUCCESS);
			QF::PUBLISH(evt, me);
			status = Q_TRAN(&Synth::Stopped);
			break;
		}
		case SYNTH_SET_MODE_PARAPHONIC_REQ: {
			LOG_EVENT(e);
			
			status = Q_TRAN(&Synth::Paraphonic);
			break;
		}
		case SYNTH_SET_MODE_MONOPHONIC_REQ: {
			LOG_EVENT(e);
			
			status = Q_TRAN(Synth::Monophonic);
			break;
		}
		case SYNTH_UPDATE_TIMER:{
			me->flush();
			
			me->startTimer();
			status = Q_HANDLED();
			break;
		}
		case SYNTH_SET_CC_HANDLER:{
			LOG_EVENT(e);
			synthSetCCHandler const &req = static_cast<synthSetCCHandler const &>(*e);
			
			me->setControlHandler(req.getCC(), req.getType(), req.getArgs(), req.getCount());
			status = Q_HANDLED();
			break;
		}
		case SYNTH_SET_LFO_TARGET_REQ:{
			LOG_EVENT(e);
			synthSetLFOTargetReq const &req = static_cast<synthSetLFOTargetReq const &>(*e);
			
			//remove the pointer from the old target
			LFO *lfo = &me->lfos[req.getNum()];
			if(lfo->target != NULL){
				//if the target is wave 0, remove from all waves
				if(lfo->target == &me->waves[0]){
					for(int i=0; i<NUM_CHANNELS; i++){
						me->waves[i].lfos[req.getNum()] = NULL;
					}
				}
				else
					lfo->target->lfos[req.getNum()] = NULL;
			}
			
			//set the new target
			lfo->targetType = req.getType();
			switch(req.getType()){
				case LFO_TARGET_PITCH:{
					wavetable *w = &me->waves[0];
					//scale to 512 values (-256 to 256)
					lfo->mapMax = 255;
					
					lfo->target = w;
					//pitch modulators actually target all waves
					for(int i=0; i<NUM_CHANNELS; i++){
						me->waves[i].lfos[req.getNum()] = lfo;
					}
					break;
				}
				case LFO_TARGET_CV:{
					cv * c = &me->cvs[req.getId()];
					//scale to 2048 values
					lfo->mapMax = 1023;
					
					lfo->target = c;
					c->lfos[req.getNum()] = lfo;
					break;
				}
				case LFO_TARGET_DEPTH:{
					LFO *l = &me->lfos[req.getId()];
					//scale to 64 values
					lfo->mapMax = 63;
					
					lfo->target = l;
					l->lfos[req.getNum()] = lfo;
					break;
				}
				case LFO_TARGET_RATE:{
					LFO *l = &me->lfos[req.getId()];
					//scale to (LFO_MAX_FREQ * LFO_NUM values) values
					lfo->mapMax = (LFO_MAX_FREQ * LFO_NUM_VALUES) >> 1;
					
					lfo->target = l;
					l->lfos[req.getNum()] = lfo;
					break;
				}
				default:
					//make sure the type can be handled
					Q_ASSERT(0);
					break;
			}
			
			status = Q_HANDLED();
			break;
		}
		case PITCH_BEND_REQ: {
			//LOG_EVENT(e);
			pitchBendReq const &req = static_cast<pitchBendReq const &>(*e);
			
			//TODO: max val on this is wrong I think
			me->bend = map(req.getValue(), -8192, 8191, -100, 100);
			
			//mark all channels to be updated
			for(int i=0; i<NUM_CHANNELS; i++){
				waves[i].UPDATE_BITS.fs = true;
			}
			
			status = Q_HANDLED();
			break;
		}
		case CONTROL_CHANGE_REQ:{
			controlChangeReq const &req = static_cast<controlChangeReq const &>(*e);
			//Q_ASSERT(req.getCC() < MAX_CC);
			if(me->cc[req.getCC()] != NULL) (me->*me->cc[req.getCC()])(req.getChannel(), req.getValue(), &me->cc_args[req.getCC()]);
			
			//log this for saving presets
			CC_LOG *c = &me->cc_log[req.getCC()];
			c->cc = req.getCC();
			c->values[req.getChannel() - 1] = req.getValue();
			
			status = Q_HANDLED();
			break;
		}
		case SYNTH_STORE_PRESET_REQ: {
			LOG_EVENT(e);
			status = Q_TRAN(&Synth::StoringPreset);
			break;
		}
		case SYNTH_LOAD_PRESET_REQ: {
			LOG_EVENT(e);
			status = Q_TRAN(&Synth::LoadingPreset);
			break;
		}
		default: {
			status = Q_SUPER(&Synth::Root);
			break;
		}
	}
	return status;
}

QState Synth::Monophonic(Synth * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			
			Evt *evt = new FPGASetEnableReq(SUB0 | NOISE_EN);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case NOTE_ON_REQ: {
			LOG_EVENT(e);
			me->noteOnMono(e);
			status = Q_HANDLED();
			break;
		}
		case NOTE_OFF_REQ: {
			LOG_EVENT(e);
			me->noteOffMono(e);
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&Synth::Started);
			break;
		}
	}
	return status;
}

QState Synth::Paraphonic(Synth * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			//in paraphonic mode all waves should have the same waveform and volume
			wavetable *base = &me->waves[0];
			//remove transpose
			base->setTranspose(0);
			for(int i=1; i<NUM_CHANNELS; i++){
				wavetable *w = &me->waves[i];
				w->setTranspose(0);
				w->stopNote();
				
				Evt *evt = new FPGAWriteWaveFile(base->waveform_number, i);
				QF::PUBLISH(evt, me);
			}
			
			//enable all subs
			Evt *evt = new FPGASetEnableReq(SUB0 | SUB1 | SUB2 | NOISE_EN);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case NOTE_ON_REQ: {
			LOG_EVENT(e);
			me->noteOnPara(e);
			status = Q_HANDLED();
			break;
		}
		case NOTE_OFF_REQ: {
			LOG_EVENT(e);
			me->noteOffPara(e);
			status = Q_HANDLED();
			break;
		}
		case CONTROL_CHANGE_REQ:{
			controlChangeReq const &req = static_cast<controlChangeReq const &>(*e);
			if(me->cc[req.getCC()] != NULL){
				//block certain CCs in paraphonic mode
				struct CC_ARG *arg = &me->cc_args[req.getCC()];
				switch(arg->type){
					case WT_TRANSPOSE:
						break;
					case WT_WAVE:
					case WT_VOLUME:{
						(me->*me->cc[req.getCC()])(1, req.getValue(), &me->cc_args[req.getCC()]);
						(me->*me->cc[req.getCC()])(2, req.getValue(), &me->cc_args[req.getCC()]);
						(me->*me->cc[req.getCC()])(3, req.getValue(), &me->cc_args[req.getCC()]);
						
						//log this for saving presets
						CC_LOG *c = &me->cc_log[req.getCC()];
						c->cc = req.getCC();
						c->values[0] = req.getValue();
						c->values[1] = req.getValue();
						c->values[2] = req.getValue();
						
						break;
					}
					default:{
						(me->*me->cc[req.getCC()])(req.getChannel(), req.getValue(), &me->cc_args[req.getCC()]);
						
						//log this for saving presets
						CC_LOG *c = &me->cc_log[req.getCC()];
						c->cc = req.getCC();
						c->values[req.getChannel() - 1] = req.getValue();
						break;
					}
						
				}
			}
			status = Q_HANDLED();
			break;
		}
		default: {
			status = Q_SUPER(&Synth::Started);
			break;
		}
	}
	return status;
}

QState Synth::LoadingPreset(Synth * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			//otherwise we're gonna have some problems
			Q_ASSERT(SD_READ_MAX % (NUM_CHANNELS + 1) == 0);
			me->resetDefaults();
			
			char num[3];
			itoa(me->cc_log[PRESET_LOAD_SELECT_CC].values[0], num, 10);
			const char *pnum = (const char *)num;
			
			char path[25];
			strcpy(path, PRESETS_PATH);
			strcat(path, "p");
			strcat(path, pnum);
			strcat(path, ".pst");
			
			Evt *evt = new SDReadFileReq(0, path, 0, SD_READ_MAX, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			//recall if we have deferred another write req
			while(me->recall(&me->m_deferQueue));
			
			status = Q_HANDLED();
			break;
		}
		case Q_INIT_SIG:{
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case SYNTH_SET_MODE_MONOPHONIC_REQ:
		case SYNTH_SET_MODE_PARAPHONIC_REQ:
		case SYNTH_LOAD_PRESET_REQ: 
		case SYNTH_STORE_PRESET_REQ: {
			LOG_EVENT(e);
			//idk lets just ignore these. what are you even doing my dude? just wait
			status = Q_HANDLED();
			break;
		}
		case SD_READ_FILE_RESPONSE: {
			LOG_EVENT(e);
			
			SDReadFileResponse const &req = static_cast<SDReadFileResponse const &>(*e);
			if(req.getBuf() == &me->SDBuffer){
				//we know this read is for us
				if(req.getError()){
					//hmm preset file probably doesn't exist
					status = Q_TRAN(&Synth::Started);
				}
				else{
					//send all the messages
					while(!me->SDBuffer.empty()){
						byte cc = me->SDBuffer.pop_front();
						
						for(int i=0; i<NUM_CHANNELS; i++){
							Evt *evt = new controlChangeReq(i + 1, cc, me->SDBuffer.pop_front());
							me->postLIFO(evt);
						}
					}
					
					if(req.getEof()){
						status = Q_TRAN(&Synth::Started);
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
			status = Q_SUPER(&Synth::Started);
			break;
		}
	}
	return status;
}

QState Synth::StoringPreset(Synth * const me, QEvt const * const e) {
	QState status;
	switch (e->sig) {
		case Q_ENTRY_SIG: {
			LOG_EVENT(e);
			me->ccStoreNum = 0;
			uint16_t numBytes = 0;
			
			//otherwise we're gonna have some problems
			Q_ASSERT(SD_READ_MAX % (NUM_CHANNELS + 1) == 0);
			
			char num[3];
			itoa(me->cc_log[PRESET_STORE_SELECT_CC].values[0], num, 10);
			const char *pnum = (const char *)num;
			
			char path[25];
			strcpy(path, PRESETS_PATH);
			strcat(path, "p");
			strcat(path, pnum);
			strcat(path, ".pst");
			
			//empty the buffer in case there's anything in there (shouldn't be)
			while(!me->SDBuffer.empty()){ me->SDBuffer.pop_front(); }
			
			// 124 - 127 are reserved for loading and storing presets
			while(!me->SDBuffer.full() && me->ccStoreNum < PRESET_LOAD_SELECT_CC){
				
				//only write CCs that are actually active
				if(me->cc[me->ccStoreNum] != NULL){
					me->SDBuffer.push_back(me->cc_log[me->ccStoreNum].cc);
					me->SDBuffer.transfer_in(me->cc_log[me->ccStoreNum].values, NUM_CHANNELS);
					numBytes += (NUM_CHANNELS + 1);
				}
				me->ccStoreNum++;
			}
			
			Evt *evt = new SDWriteFileReq(0, path, 0, numBytes, &me->SDBuffer);
			QF::PUBLISH(evt, me);
			
			status = Q_HANDLED();
			break;
		}
		case Q_EXIT_SIG: {
			LOG_EVENT(e);
			
			while(me->recall(&me->m_deferQueue));
			
			status = Q_HANDLED();
			break;
		}
		case Q_INIT_SIG:{
			LOG_EVENT(e);
			status = Q_HANDLED();
			break;
		}
		case SYNTH_SET_MODE_MONOPHONIC_REQ:
		case SYNTH_SET_MODE_PARAPHONIC_REQ:
		case SYNTH_LOAD_PRESET_REQ:
		case SYNTH_STORE_PRESET_REQ:{
			LOG_EVENT(e);
			//defer this until we are done writing the preset
			me->defer(&me->m_deferQueue, e);
			break;
		}
		case SD_WRITE_FILE_RESPONSE: {
			LOG_EVENT(e);
			uint16_t numBytes = 0;
			
			SDWriteFileResponse const &req = static_cast<SDWriteFileResponse const &>(*e);
			if(req.getBuf() == &me->SDBuffer){
				// 124 - 127 are reserved for loading and storing presets
				while(!me->SDBuffer.full() && me->ccStoreNum < PRESET_LOAD_SELECT_CC){
					
					//only write CCs that are actually active
					if(me->cc[me->ccStoreNum] != NULL){
						me->SDBuffer.push_back(me->cc_log[me->ccStoreNum].cc);
						me->SDBuffer.transfer_in(me->cc_log[me->ccStoreNum].values, NUM_CHANNELS);
						numBytes += (NUM_CHANNELS + 1);
					}
					me->ccStoreNum++;
				}
				if(me->SDBuffer.empty()){
					//nothing more to write
					status = Q_TRAN(&Synth::Started);
				}
				else{
					Evt *evt = new SDWriteFileReq(0, req.getFilename(), req.getExitPos(), numBytes, &me->SDBuffer);
					QF::PUBLISH(evt, me);
				
					status = Q_HANDLED();
				}
			}
			break;
		}
		default: {
			status = Q_SUPER(&Synth::Started);
			break;
		}
	}
	return status;
}

void Synth::flush(){
	Evt* evt;
	
	//update any LFOs
	for(int i=0; i<NUM_LFO; i++){
		lfos[i].writeUpdates();
		
		if(lfos[i].UPDATE_BITS.wave){
			Evt *evt = new FlashConfigReadLFOReq(lfos[i].waveform_number, lfos[i].values);
			QF::PUBLISH(evt, me);
			lfos[i].UPDATE_BITS.wave = false;
			lfos[i].activate();
		}
	}
	
	for(int i=0; i<NUM_CHANNELS; i++){
		wavetable *w = &waves[i];
		int net = 0;
		//account for any lfos
		for(int j=0; j<NUM_LFO; j++){
			LFO *lfo = w->lfos[j];
			if(lfo != NULL && lfo->active()){
				net += lfo->CURRENT_VALUE;
			}
		}
		if(w->FS_NET != net){
			w->UPDATE_BITS.fs = true;
			w->FS_NET = net;
		}
		//write any necessary updates
		if(w->UPDATE_BITS.fs){
			uint16_t fs;
			if(w->FS_BASE == FS_NOTE_OFF) fs = 0;
			else fs = periodToFS[w->FS_BASE + w->FS_NET + bend];
			evt = new FPGAWriteFS(i, fs);
			QF::PUBLISH(evt, this);
			
			w->UPDATE_BITS.fs = false;
		}
		if(w->UPDATE_BITS.wave){
			Evt *evt;
			evt = new FPGAWriteWaveFile(w->waveform_number, i);
			QF::PUBLISH(evt, me);
			w->UPDATE_BITS.wave = false;
		}
	}
	
	for(int i=0; i<NUM_CV; i++){
		cv *c = &cvs[i];
		int net = 0;
		//account for any lfos
		for(int j=0; j<NUM_LFO; j++){
			LFO *lfo = c->lfos[j];
			if(lfo != NULL && lfo->active()){
				net += lfo->CURRENT_VALUE;
			}
		}
		if(c->NET != net){
			c->UPDATE = true;
			c->NET = net;
		}
		//write any necessary updates
		if(c->UPDATE){
			evt = new FPGAWritePWM(i, constrain(c->BASE + c->NET, c->low, c->high));
			QF::PUBLISH(evt, this);
			
			c->UPDATE = false;
		}
	}
	
	//update ADSR
	if(adsr.NEEDS_UPDATE){
		if(adsr.UPDATE_BITS.attack){
			evt = new FPGAWriteParamReq(ATTACK, adsr.A_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.attack = 0;
		}
		if(adsr.UPDATE_BITS.decay){
			evt = new FPGAWriteParamReq(DECAY, adsr.D_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.decay = 0;
		}
		if(adsr.UPDATE_BITS.sustain){
			evt = new FPGAWriteParamReq(SUSTAIN, adsr.S_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.sustain = 0;
		}
		if(adsr.UPDATE_BITS.release){
			evt = new FPGAWriteParamReq(RELEASE, adsr.R_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.release = 0;
		}
		if(adsr.UPDATE_BITS.cutoff_mix){
			evt = new FPGAWriteParamReq(CUTOFF_MIX, adsr.MIX_CUTOFF_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.cutoff_mix = 0;
		}
		if(adsr.UPDATE_BITS.amp_mix){
			evt = new FPGAWriteParamReq(AMP_MIX, adsr.MIX_AMP_BASE);
			QF::PUBLISH(evt, this);
			adsr.UPDATE_BITS.amp_mix = 0;
		}
	}
}

void Synth::startTimer(){
	enableTimer(SYNTH_TC);
}

void Synth::stopTimer(){
	disableTimer(SYNTH_TC);
}

void Synth::timerCallback(){
	stopTimer();
	Evt *evt = new Evt(SYNTH_UPDATE_TIMER);
	QF::PUBLISH(evt, 0);
}

void Synth::LFOFireCallback(uint8_t num){
	lfos[num].process();
}

void Synth::resetDefaults(){
	for(int i=0; i<NUM_CHANNELS; i++){
		waves[i].reset();
	}
	for(int i=0; i<NUM_LFO; i++){
		lfos[i].reset();
	}
}

void Synth::killNotes(){
	for(int i=0; i<NOTE_BUF_MAX; i++){
		free(notebuf[i]);
		notebuf[i] = NULL;
	}
	for(int i=0; i<NUM_CHANNELS; i++){
		waves[i].stopNote();
	}
}

/************ NOTE HANDLERS **********/
void Synth::noteOnMono(const QEvt *e){
	if(NOTE_COUNT < NOTE_BUF_MAX){
		noteOnReq const &req = static_cast<noteOnReq const &>(*e);
		//GLOBAL_NOTE_PRESSED = true;
		
		struct note *n = new note();
		n->channel = req.getChannel();
		n->pitch = req.getNote();
		n->velocity = req.getVelocity();
		
		//find a slot for the new note
		for(int i=0; i<NOTE_BUF_MAX; i++){
			if(notebuf[i] == NULL){
				notebuf[i] = n;

				for(int j=0; j<NUM_CHANNELS; j++){
					waves[j].setNote(n);
				}
				NOTE_COUNT++;
				if(NOTE_COUNT == 1) {
					//send key pressed
					Evt *evt = new FPGANotifyKeyPressed(true);
					QF::PUBLISH(evt, this);
				}
				break;
			}
		}
	}
}

void Synth::noteOnPara(const QEvt *e){
	
	//stop all old notes if the buffer is empty
	if(NOTE_COUNT == 0){
		for(int j=0; j<NUM_CHANNELS; j++){
			waves[j].stopNote();
		}
	}
	if(NOTE_COUNT < NOTE_BUF_MAX){
		noteOnReq const &req = static_cast<noteOnReq const &>(*e);
		//GLOBAL_NOTE_PRESSED = true;
		
		struct note *n = new note();
		n->channel = req.getChannel();
		n->pitch = req.getNote();
		n->velocity = req.getVelocity();
		
		//find a slot for the new note
		for(int i=0; i<NOTE_BUF_MAX; i++){
			if(notebuf[i] == NULL){
				notebuf[i] = n;
				
				//check for an open channel
				for(int j=0; j<NUM_CHANNELS; j++){
					if(waves[j].CurrentNote == NULL){
						n->activeOn = &waves[j];
						waves[j].setNote(n);
						break;
					}
				}
				NOTE_COUNT++;
				if(NOTE_COUNT == 1) {
					Evt *evt = new FPGANotifyKeyPressed(true);
					QF::PUBLISH(evt, this);
				}
				break;
			}
		}
	}
}

void Synth::noteOffMono(const QEvt *e){
	if(NOTE_COUNT > 0){
		noteOffReq const &req = static_cast<noteOffReq const &>(*e);
		struct note *n;
		for(int i=0; i<NOTE_BUF_MAX; i++){
			if(notebuf[i] != NULL){
				n = notebuf[i];
				if(n->pitch == req.getNote()){
					free(n);
					notebuf[i] = NULL;
					NOTE_COUNT--;
					
					//set to another note if one is still active
					if(NOTE_COUNT > 0){
						for(int j=0; j<NOTE_BUF_MAX; j++){
							if(notebuf[j] != NULL){
								//set on all channels
								for(int k=0; k<NUM_CHANNELS; k++){
									waves[k].setNote(notebuf[j]);
								}
								break;
							}
						}
					}
					break;
				}
			}
		}
	}
	if(NOTE_COUNT == 0) {
		//send key pressed
		Evt *evt = new FPGANotifyKeyPressed(false);
		QF::PUBLISH(evt, this);
	}
}

void Synth::noteOffPara(const QEvt *e){
	if(NOTE_COUNT > 0){
		noteOffReq const &req = static_cast<noteOffReq const &>(*e);
		struct note *n;
		for(int i=0; i<NOTE_BUF_MAX; i++){
			if(notebuf[i] != NULL){
				n = notebuf[i];
				if(n->pitch == req.getNote()){
					//paraphonic mode, stop channel the note is on.
					//if there is another note on deck, assign it to the channel
					wavetable *wt = n->activeOn;
					free(n);
					notebuf[i] = NULL;
					NOTE_COUNT--;
					
					if(wt != NULL){
						wt->CurrentNote = NULL;
						if(NOTE_COUNT > 0){
							wt->stopNote();
							for(int j=0; j<NOTE_BUF_MAX; j++){
								if(notebuf[j] != NULL){
									struct note *nn = notebuf[j];
									if(nn->activeOn == NULL){
										nn->activeOn = wt;
										wt->setNote(nn);
										break;
									}
								}
							}
						}
					}
					break;
				}
			}
		}
	}
	if(NOTE_COUNT == 0) {
		//send key pressed
		Evt *evt = new FPGANotifyKeyPressed(false);
		QF::PUBLISH(evt, this);
	}
}

/************* CC STUFF **************/

void Synth::setControlHandler(byte number, ccType_t type, byte *args, int count){
	
	Q_ASSERT(count <= CC_ARGS_MAX_LENGTH);
	
    cc_args[number].count = count;
    cc_args[number].type = type;
    //copy the arguments. These will be passed in to the callback.
    memcpy(cc_args[number].args, args, count);
    
    switch(type){
        case LFO_RATE:
            cc[number] = &Synth::cc_LFO_rate;
            break;
        case LFO_DEPTH:
            cc[number] = &Synth::cc_LFO_depth;
            break;
        case CV:
            /* period of a PWM output. Args will be pwm number. */
            cc[number] = &Synth::cc_cv;
            break;
        case WT_TUNE:
            /* fine tuning of a channel. Args will be channel num */
            cc[number] = &Synth::cc_tune;
            break;
        case WT_TRANSPOSE:
            /* coarse tuning of a channel. Args will be channel num */
            cc[number] = &Synth::cc_transpose;
            break;
        case WT_VOLUME:
            /* volume multiplier of a channel. Args will be channel num */
            cc[number] = &Synth::cc_volume;
            break;
        case WT_SUB:
            /* sub enable disable per osc */
            cc[number] = &Synth::cc_sub;
            break;
        case PARA_MODE:
            /* enable/disable paraphonic mode */
            cc[number] = &Synth::cc_para;
            break;
        case GLIDE_TIME:
            /* set glide rate */
            cc[number] = &Synth::cc_glide;
            break;
		case ENV:
			/* set an envelope param */
			cc[number] = &Synth::cc_env;
			break;
		case WT_WAVE:
			/* set a default waveform */
			cc[number] = &Synth::cc_wave;
			break;
		case LFO_WAVE:
			/* set a default waveform */
			cc[number] = &Synth::cc_LFO_wave;
			break;
		case CC_LFO_TARGET:
			/* set a default waveform */
			cc[number] = &Synth::cc_LFO_target;
			break;
		case PRESET_LOAD_SELECT:
			/* dont do anything rly */
			cc[number] = &Synth::cc_dummy;
			break;
		case PRESET_STORE_SELECT:
			/* dont do anything rly */
			cc[number] = &Synth::cc_dummy;
			break;
		case PRESET_LOAD_ENTER:
			/* load a preset */
			cc[number] = &Synth::cc_load;
			break;
		case PRESET_STORE_ENTER:
			/* store a preset */
			cc[number] = &Synth::cc_store;
			break;
        default:
            //this is not a valid type.
            cc[number] = NULL;
			Q_ASSERT(0);
            break;
    }
}

//*********** CC PROCESSOR FUNCTIONS *****************//
void Synth::cc_LFO_rate(byte channel, byte value, struct CC_ARG *args){
	if(channel <= NUM_LFO){
		lfos[channel - 1].setRate(map(value, 0, 127, 0, 20 * LFO_NUM_VALUES));
	}
}

void Synth::cc_LFO_depth(byte channel, byte value, struct CC_ARG *args){
    if(channel <= NUM_LFO){
		lfos[channel - 1].setDepth(value);
	}
}

void Synth::cc_LFO_wave(byte channel, byte value, struct CC_ARG *args){
	
	if(channel <= NUM_LFO){
		lfos[channel - 1].waveform_number = value;
		lfos[channel - 1].UPDATE_BITS.wave = true;
	}
}

void Synth::cc_LFO_target(byte channel, byte value, struct CC_ARG *args){
	
	if(channel <= NUM_LFO){
	
		uint8_t targetType = LFO_TARGET_NONE;
		uint8_t targetId;
	
		switch(value){
			case 1:
				targetType = LFO_TARGET_PITCH;
				targetId = 0;
				break;
			case 2:
				targetType = LFO_TARGET_CV;
				targetId = CUTOFF;
				break;
			case 3:
				targetType = LFO_TARGET_CV;
				targetId = AMP;
				break;
			case 4:
				targetType = LFO_TARGET_CV;
				targetId = RESONANCE;
				break;
			case 5:
				targetType = LFO_TARGET_CV;
				targetId = NOISE;
				break;
			case 6:
				targetType = LFO_TARGET_CV;
				targetId = SUB;
				break;
			case 7:
				targetType = LFO_TARGET_RATE;
				targetId = (channel == 1 ? 1 : 0);
				break;
			case 8:
				targetType = LFO_TARGET_DEPTH;
				targetId = (channel == 1 ? 1 : 0);
				break;
			default:
				break;
		}
		if(targetType != LFO_TARGET_NONE){
			Evt *evt;
			evt = new synthSetLFOTargetReq(channel - 1, targetType, targetId);
			this->postLIFO(evt);
		}
	}
}

void Synth::cc_cv(byte channel, byte value, struct CC_ARG *args){
    //get the arguments. There should be only one integer value
	if(channel == 1){
		struct CC_ARG *a = args;
		int const &pwm = static_cast<int const &>(*a->args);
		
		cvs[pwm].setBase(value);
	}
}

void Synth::cc_tune(byte channel, byte value, struct CC_ARG *args){
    if(channel <= NUM_CHANNELS){
		waves[channel - 1].setTune(map(value, 0, 127, -50, 50));
	}
}

void Synth::cc_transpose(byte channel, byte value, struct CC_ARG *args){
    if(channel <= NUM_CHANNELS){
		waves[channel - 1].setTranspose(map(value, 0, 127, -24, 24));
	}
}

void Synth::cc_volume(byte channel, byte value, struct CC_ARG *args){
    if(channel <= NUM_CHANNELS){
		uint8_t vol = map(value, 0, 127, 7, 0);
		Evt *evt = new FPGAWriteVolReq(channel, vol);
		QF::PUBLISH(evt, me);
	}
}

void Synth::cc_sub(byte channel, byte value, struct CC_ARG *args){
	if(channel <= NUM_CHANNELS){
		struct CC_ARG *a = args;
		//int channel = (*(int *)a->args);
    
		/*
		//TODO: future version needs to have miso on fpga, we need to know state of sub reg
		if(value > 63) writeReg(BFX_SUB_ENABLE, channel);
		else writeReg(BFX_SUB_ENABLE, BFX_SUB_OFF);
		*/
	}
}
void Synth::cc_para(byte channel, byte value, struct CC_ARG *args){
	if(channel == 1){
		Evt *evt;
		if(value > 63)
		evt = new Evt(SYNTH_SET_MODE_PARAPHONIC_REQ);
		else
		evt = new Evt(SYNTH_SET_MODE_MONOPHONIC_REQ);
		
		this->postLIFO(evt);
	}
}
void Synth::cc_glide(byte channel, byte value, struct CC_ARG *args){
	if(channel == 1){
		int glide = map(value, 0, 127, 0, 65535);
		
		Evt *evt = new FPGASetPortamentoReq(glide);
		QF::PUBLISH(evt, this);
	}
}
void Synth::cc_env(byte channel, byte value, struct CC_ARG *args){
	if(channel == 1){
		//get the arguments. There should be only one integer value
		struct CC_ARG *a = args;
		int const &param = static_cast<int const &>(*a->args);
	
		switch(param){
			case CUTOFF_MIX:
			case AMP_MIX:
				adsr.setParam(param, map(value, 0, 127, 0, 32));
				break;
			case SUSTAIN:
				adsr.setParam(param, value);
				break;
			default:
				uint32_t val = linToLog22[value];
				adsr.setParam(param, val);
				break;
		}
	}
}

void Synth::cc_wave(byte channel, byte value, struct CC_ARG *args){
	
	if(channel <= NUM_CHANNELS){
		waves[channel - 1].setWave(value);
	}
}

void Synth::cc_load(byte channel, byte value, struct CC_ARG *args){
	Evt *evt;
	if(value == 127){
		 evt = new Evt(SYNTH_LOAD_PRESET_REQ);
		this->postLIFO(evt);
	}
}

void Synth::cc_store(byte channel, byte value, struct CC_ARG *args){
	Evt *evt;
	if(value == 127){
		 evt = new Evt(SYNTH_STORE_PRESET_REQ);
		 this->postLIFO(evt);
	}
}