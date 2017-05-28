#ifndef LIB_SYNTH_H_
#define LIB_SYNTH_H_

#include <inttypes.h>
#include "qpcpp.h"
#include "qp_extras.h"
#include "Arduino.h"
#include "SynthDefs.h"
#include "SynthExtras.h"
#include "buffer.h"

#include "hsm_id.h"

using namespace QP;
using namespace FW;

#define NOTE_BUF_MAX	6

#define SYNTH_TC		TC3
#define SYNTH_UPDATE_FREQ 200

class Synth : public QActive {
	
	public:
	Synth();
	~Synth();
	
	void Start(uint8_t prio);
	
	static void timerCallback();
	static void LFOFireCallback(uint8_t num);
	static void stopTimer();
	
	protected:
	static QState InitialPseudoState(Synth * const me, QEvt const * const e);
	static QState Root(Synth * const me, QEvt const * const e);
	static QState Stopped(Synth * const me, QEvt const * const e);
	static QState Started(Synth * const me, QEvt const * const e);
	static QState LoadingPreset(Synth * const me, QEvt const * const e);
	static QState StoringPreset(Synth * const me, QEvt const * const e);
	
	static QState Monophonic(Synth * const me, QEvt const * const e);
	static QState Paraphonic(Synth * const me, QEvt const * const e);
	
	enum {
		EVT_QUEUE_COUNT = 100,
		DEFER_QUEUE_COUNT = 10
	};
	QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
	QEvt const *m_deferQueueStor[DEFER_QUEUE_COUNT];
	QEQueue m_deferQueue;
	uint8_t m_id;
	char const * m_name;
	
	QTimeEvt m_waveformTimer;
	
private:
	void flush();
	void startTimer();
	
	buffer SDBuffer;
	
	static struct note *notebuf[NOTE_BUF_MAX];
	int NOTE_COUNT = 0;
	
	static struct CC_LOG cc_log[MAX_CC];
	
	static wavetable waves[NUM_CHANNELS];
	static cv cvs[NUM_CV];
	static LFO lfos[NUM_LFO];
	ADSR adsr;
	
	void (Synth::*cc[MAX_CC])(byte channel, byte value, struct CC_ARG *);
	struct CC_ARG cc_args[MAX_CC];
	
	void resetDefaults();
	void killNotes();
	
	void noteOnMono(const QEvt *e);
	void noteOnPara(const QEvt *e);
	void noteOffMono(const QEvt *e);
	void noteOffPara(const QEvt *e);
	    
	void setControlHandler(byte number, ccType_t type, byte *args, int count);
	
	void cc_LFO_rate(byte channel, byte value, struct CC_ARG *args);
	void cc_LFO_depth(byte channel, byte value, struct CC_ARG *args);
	void cc_LFO_wave(byte channel, byte value, struct CC_ARG *args);
	void cc_LFO_target(byte channel, byte value, struct CC_ARG *args);
	void cc_cv(byte channel, byte value, struct CC_ARG *args);
	void cc_tune(byte channel, byte value, struct CC_ARG *args);
	void cc_transpose(byte channel, byte value, struct CC_ARG *args);
	void cc_volume(byte channel, byte value, struct CC_ARG *args);
	void cc_sub(byte channel, byte value, struct CC_ARG *args);
	void cc_para(byte channel, byte value, struct CC_ARG *args);
	void cc_glide(byte channel, byte value, struct CC_ARG *args);
	void cc_env(byte channel, byte value, struct CC_ARG *args);
	void cc_wave(byte channel, byte value, struct CC_ARG *args);
	void cc_dummy(byte channel, byte value, struct CC_ARG *args) {}
	void cc_load(byte channel, byte value, struct CC_ARG *args);
	void cc_store(byte channel, byte value, struct CC_ARG *args);
	
	int bend = 0;
	
	uint8_t LFOWritingNum;
	uint8_t writePos;
	uint8_t ccStoreNum;
	
};

#endif