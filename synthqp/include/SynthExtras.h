#ifndef LIB_SYNTH_EXTRAS_H
#define LIB_SYNTH_EXTRAS_H

#include "Arduino.h"
#include "bsp.h"
#include "SynthDefs.h"

#define FS_NOTE_OFF 0xFFFF

class wavetable;
class LFO;

struct note{
	byte channel;
	byte pitch;
	byte velocity;
	wavetable *activeOn = NULL;
};

struct CC_ARG{
	byte count;
	byte args[CC_ARGS_MAX_LENGTH];
	ccType_t type;
};

//these will be stored when we get CC messages and used to write presets
struct CC_LOG{
	byte cc;
	byte values[NUM_CHANNELS] = {0, 0, 0};
};

class LFO_TARGET{
public:
	LFO_TARGET() : lfos{NULL, NULL} {}
	~LFO_TARGET() {}
		
	LFO *lfos[NUM_LFO];	
	
	void reset(){
		for(int i=0; i<NUM_LFO; i++){
			lfos[i] = NULL;
		}
	};
};

class LFO : public LFO_TARGET {
public:
	LFO(Tc *TC) : LFO_TARGET(), TC(TC) {};
	~LFO() {};
		
	volatile int CURRENT_VALUE = 0;
		
	struct ud_bits {
		uint8_t rate = 1;
		uint8_t depth = 0;
		uint8_t wave = 0;
	};
	struct LFO::ud_bits UPDATE_BITS;
		
	uint16_t RATE_BASE = 1;
	int RATE_NET = 0;
	uint16_t DEPTH_BASE = 0;
	int DEPTH_NET = 0;
	volatile uint8_t POS = 0;
	
	signed short values[LFO_NUM_VALUES];
	
	//we will need to reference these when changing targets
	LFO_TARGET *target = NULL;
	uint8_t targetType = LFO_TARGET_NONE;
	
	uint8_t waveform_number = 2;
	
	int mapMax = 32767;
	
	bool active(){ return ACTIVE; }
	
	void setRate(uint16_t freq){
		RATE_BASE = freq;
		UPDATE_BITS.rate = true;
	}
	
	void setDepth(uint8_t d){
		DEPTH_BASE = d;
		UPDATE_BITS.depth = true;
	}
	
	void process(){
		POS++;
		CURRENT_VALUE = map(values[POS], -32768, 32767, 0 - depth, depth);
	}
	
	void activate(){
		enableTimer(TC);
		ACTIVE = true;
	}
	
	void deactivate(){
		disableTimer(TC);
		ACTIVE = false;
	}
	
	void writeUpdates(){
		int net_rate = 0;
		int net_depth = 0;
		for(int i=0; i<NUM_LFO; i++){
			LFO *lfo = this->lfos[i];
			if(lfo != NULL && lfo->active()){
				if(lfo->targetType == LFO_TARGET_RATE)
					net_rate += lfo->CURRENT_VALUE;
				else if(lfo->targetType == LFO_TARGET_DEPTH)
					net_depth += lfo->CURRENT_VALUE;
			}
		}
		if(DEPTH_NET != net_depth){
			DEPTH_NET = net_depth;
			UPDATE_BITS.depth = true;
		}
		if(RATE_NET != net_rate){
			RATE_NET = net_rate;
			UPDATE_BITS.rate = true;
		}
		if(UPDATE_BITS.depth){
			depth = map(DEPTH_BASE + DEPTH_NET, 0, 127, 0, mapMax);
			depth = constrain(depth, 0, mapMax);
			UPDATE_BITS.depth = false;
		}
		if(UPDATE_BITS.rate){
			uint16_t rate = constrain(RATE_BASE + RATE_NET, 0, LFO_MAX_FREQ * LFO_NUM_VALUES);
			setTimerFreq(TC, rate);
			UPDATE_BITS.rate = false;
		}
	}
	
	void reset(){
		target = NULL;
		targetType = LFO_TARGET_NONE;
		
		LFO_TARGET::reset();
	}
	
private:
	Tc *TC;
	bool ACTIVE = false;
	
	int depth = 0; 
	
};

class cv : public LFO_TARGET {
	public:
	cv() : LFO_TARGET() {};
	cv(const uint16_t *values) : LFO_TARGET(), values(values) {};
	cv(uint16_t low, uint16_t high) : LFO_TARGET(), low(low), high(high) {};
	~cv() {};
	
	uint16_t BASE = 0;
	int NET = 0;
	bool UPDATE = false;
	
	uint16_t low = 0;
	uint16_t high = 4095;
	const uint16_t *values = NULL;
	
	void setBase(uint8_t base){
		BASE = map(base, 0, 127, low, high);
		UPDATE = true;
	}
	
	uint16_t getCurrentValue(){
		if(this->values != NULL){
			uint8_t ix = constrain(map(this->BASE + this->NET, 0, 4095, 0, 127), 0, 127);
			return this->values[ix];
		}
		else return constrain(this->BASE + this->NET, this->low, this->high);
	}
};

class ADSR : public LFO_TARGET {
public:
	ADSR() : LFO_TARGET() {};
	~ADSR() {};
		
	bool NEEDS_UPDATE = false;
	struct ud_bits {
		uint8_t attack = 0;
		uint8_t decay = 0;
		uint8_t sustain = 0;
		uint8_t release = 0;
		uint8_t	cutoff_mix = 0;
		uint8_t amp_mix = 0;
	};
	struct ADSR::ud_bits UPDATE_BITS;
	
	uint32_t A_BASE = 0;
	uint32_t D_BASE = 0;
	uint16_t S_BASE = 0;
	uint32_t R_BASE = 0;
	uint16_t MIX_CUTOFF_BASE = 0;
	uint16_t MIX_AMP_BASE = 0;
	
	void setParam(uint8_t param, uint32_t interval){
		switch(param){
			case ATTACK:
				A_BASE = interval;
				UPDATE_BITS.attack = 1;
				break;
			case DECAY:
				D_BASE = interval;
				UPDATE_BITS.decay = 1;
				break;
			case SUSTAIN:
				S_BASE = interval;
				UPDATE_BITS.sustain = 1;
				break;
			case RELEASE:
				R_BASE = interval;
				UPDATE_BITS.release = 1;
				break;
			case CUTOFF_MIX:
				MIX_CUTOFF_BASE = interval;
				UPDATE_BITS.cutoff_mix = 1;
				break;
			case AMP_MIX:
				MIX_AMP_BASE = interval;
				UPDATE_BITS.amp_mix = 1;
				break;
			default:
				break;
		}
		NEEDS_UPDATE = true;
	}
};

class wavetable : public LFO_TARGET {
public:

	wavetable() : LFO_TARGET() {};
	~wavetable() {};
	
	struct ud_bits {
		uint8_t fs = 0;
		uint8_t wave = 0;
	};
	struct wavetable::ud_bits UPDATE_BITS;
	
	struct note *CurrentNote = NULL;
	
	uint32_t FS_BASE = 0;
	int FS_NET = 0;
	
	int TUNE = 0;						//for detuning
	int TRANSPOSE = 0;

	uint8_t waveform_number = 1;
	
	void setNote(struct note *n){ 
		CurrentNote = n;
		//get FS from note to FS lookup table, add tune
		int transpose = (TRANSPOSE * 50);
		uint32_t newBase = (n->pitch - 12) * 50 + transpose + TUNE;
		if(newBase != FS_BASE){
			FS_BASE = newBase;
			UPDATE_BITS.fs = true;
		}
	};
	
	void stopNote(){
		FS_BASE = FS_NOTE_OFF;
		CurrentNote = NULL;
		UPDATE_BITS.fs = true;
	};
	
	void setTune(int tune){
		TUNE = tune;
		setNote(CurrentNote);
	};
	
	void setTranspose(int transpose){
		TRANSPOSE = transpose;
		setNote(CurrentNote);
	};
	void setWave(uint8_t num){
		waveform_number = num;
		UPDATE_BITS.wave = true;
	}
};

#endif