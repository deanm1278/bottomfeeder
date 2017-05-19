#ifndef LIB_SYNTH_DEFS_H
#define LIB_SYNTH_DEFS_H

#define CC_ARGS_MAX_LENGTH 8
#define LFO_NUM_VALUES 256
#define LFO_MAX_FREQ 20

#define NUM_LFO			2

enum {
	SUB0 = 0x01,
	SUB1 = 0x02,
	SUB2 = 0x04,
	NOISE_EN = 0x08	
};

//lfo targets
enum {
	LFO_TARGET_NONE,
	LFO_TARGET_PITCH,
	LFO_TARGET_CV,
	LFO_TARGET_RATE,
	LFO_TARGET_DEPTH
};

//CV numbers
enum {
	CUTOFF = 0x00,
	RESONANCE = 0x01,
	AMP = 0x02,
	NOISE = 0x03,
	SUB = 0x04
};

//ADSR params
enum {
	ATTACK = 0,
	DECAY = 1,
	RELEASE = 2,
	SUSTAIN = 3,
	CUTOFF_MIX = 4,
	AMP_MIX = 5
};

typedef enum ccType_t{
	LFO_RATE = 0x00,
	LFO_DEPTH = 0x01,
	CV = 0x03,
	WT_WAVE = 0x04,
	WT_TUNE = 0x05,
	WT_TRANSPOSE = 0x06,
	WT_VOLUME = 0x07,
	WT_SUB = 0x08,
	PARA_MODE = 0x09,
	GLIDE_TIME = 0x0A,
	ENV = 0x0B
};

//default waves
enum {
	SAW = 1,
	SINE = 2,
	SQUARE = 3,
	TRIANGLE = 4	
};

#endif