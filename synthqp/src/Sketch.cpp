/***************************


****************************/

#include <Arduino.h>

#include "qpcpp.h"
#include "qp_extras.h"

#include "System.h"
#include "MIDIUART.h"
#include "FlashConfig.h"
#include "SDCard.h"
#include "FPGA.h"
#include "Synth.h"
#include "Panel.h"

#ifdef CONFIG_MIDI_USB
	#include "MIDIUSB.h"
#endif

#include "event.h"
#include "bsp.h"

enum {
    EVT_SIZE_SMALL = 32,
    EVT_SIZE_MEDIUM = 64,
    EVT_SIZE_LARGE = 256,
    EVT_COUNT_SMALL = 128,
    EVT_COUNT_MEDIUM = 16,
    EVT_COUNT_LARGE = 4
};
uint32_t evtPoolSmall[ROUND_UP_DIV_4(EVT_SIZE_SMALL * EVT_COUNT_SMALL)];
uint32_t evtPoolMedium[ROUND_UP_DIV_4(EVT_SIZE_MEDIUM * EVT_COUNT_MEDIUM)];
uint32_t evtPoolLarge[ROUND_UP_DIV_4(EVT_SIZE_LARGE * EVT_COUNT_LARGE)];
QP::QSubscrList subscrSto[MAX_PUB_SIG];

static System sys;
static MIDI_Class midi;
static FlashConfig flash;
static SDCard sd;
static FPGA fpga;

#ifdef CONFIG_MIDI_USB
static MIDIUSB midiusb;
#endif

static Synth synth;
static Panel panel;

void setup() {
  
  //for LED
  pinMode(13, OUTPUT);
  // Initialize QP. It must be done before BspInit() since the latter may enable
  // SysTick which will cause the scheduler to run.
  QF::init();
  QF::poolInit(evtPoolSmall, sizeof(evtPoolSmall), EVT_SIZE_SMALL);
  QF::poolInit(evtPoolMedium, sizeof(evtPoolMedium), EVT_SIZE_MEDIUM);
  QF::poolInit(evtPoolLarge, sizeof(evtPoolLarge), EVT_SIZE_LARGE);
  QP::QF::psInit(subscrSto, Q_DIM(subscrSto)); // init publish-subscribe
  
  BspInit();
  
  //Start active objects.
  sys.Start(PRIO_SYSTEM);
  midi.Start(PRIO_MIDI_UART);
  flash.Start(PRIO_FLASH_CONFIG);
  sd.Start(PRIO_SD_CARD);
  fpga.Start(PRIO_FPGA);
#ifdef CONFIG_MIDI_USB
  midiusb.Start(PRIO_MIDI_USB);
#endif
  synth.Start(PRIO_SYNTH);
#ifdef PANEL_ATTACHED
  panel.Start(PRIO_PANEL);
#endif
  
  //publish a start request
  Evt *evt = new SystemStartReq(0);
  QF::PUBLISH(evt, dummy);
  
  QP::QF::run();
}

extern "C" void TC3_Handler() {
	QXK_ISR_ENTRY();
	TcCount16* TC = (TcCount16*) TC3;
	
	Synth::timerCallback();
	Panel::timerCallback();
	
	// If this interrupt is due to the compare register matching the timer count
	if (TC->INTFLAG.bit.MC0 == 1) {
		TC->INTFLAG.bit.MC0 = 1;
	}
	QXK_ISR_EXIT();
}

extern "C" void TC4_Handler() {
	QXK_ISR_ENTRY();
	TcCount16* TC = (TcCount16*) TC4;
	
	Synth::LFOFireCallback(0);
	
	// If this interrupt is due to the compare register matching the timer count
	if (TC->INTFLAG.bit.MC0 == 1) {
		TC->INTFLAG.bit.MC0 = 1;
	}
	QXK_ISR_EXIT();
}

//removed from Tone.cpp in the core
extern "C" void TC5_Handler() {
	QXK_ISR_ENTRY();
	TcCount16* TC = (TcCount16*) TC5;
	
	Synth::LFOFireCallback(1);
	
	// If this interrupt is due to the compare register matching the timer count
	if (TC->INTFLAG.bit.MC0 == 1) {
		TC->INTFLAG.bit.MC0 = 1;
	}
	QXK_ISR_EXIT();
}

//TODO: MAKE SURE YOU PUT THIS BACK IN variant.cpp IN THE CORE
extern "C" void SERCOM5_Handler(){
	QXK_ISR_ENTRY();
	Serial.IrqHandler();
	MIDI_Class::RxCallback(MIDI_UART);
	QXK_ISR_EXIT();
}

extern "C" void HardFault_Handler(){
	__BKPT();
	for(;;);
}


