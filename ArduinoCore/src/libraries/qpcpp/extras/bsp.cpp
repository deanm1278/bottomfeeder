///***************************************************************************
// Product: DPP example, STM32 NUCLEO-L152RE board, preemptive QK kernel
// Last updated for version 5.6.5
// Last updated on  2016-07-05
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// http://www.state-machine.com
// mailto:info@state-machine.com
//****************************************************************************

#include <string.h>
#include "qpcpp.h"
#include "bsp.h"
#include "Arduino.h"
#include "wiring_private.h"

//Q_DEFINE_THIS_FILE

#define ENABLE_BSP_PRINT

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

volatile bool initialized = false;

//this is for the arduino core, but it actually never runs
void loop() {
}

void configureCTInt(){
	uint32_t config, pos;
	uint32_t pin = digitalPinToInterrupt(8);
		
	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_EIC));
	EIC->CTRL.bit.ENABLE = 1;
	while (EIC->STATUS.bit.SYNCBUSY == 1) { }
		
	#if ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10606
	EExt_Interrupts in = g_APinDescription[pin].ulExtInt;
	#else
	EExt_Interrupts in = digitalPinToInterrupt(pin);
	#endif
		
	// Enable wakeup capability on pin in case being used during sleep
	EIC->WAKEUP.reg |= (1 << in);

	// Assign pin to EIC
	pinPeripheral(pin, PIO_EXTINT);
		
	if (in > EXTERNAL_INT_7) {
		config = 1;
		} else {
		config = 0;
	}
		
	pos = (in - (8 * config)) << 2;
	//LOW
	EIC->CONFIG[config].reg |= EIC_CONFIG_SENSE0_LOW_Val << pos;
		
	EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << in);
}

void enableTimer(Tc *TMR){
	TcCount16* TC = (TcCount16*)TMR;
	
	TC->CTRLA.reg |= TC_CTRLA_ENABLE;
	while (TC->STATUS.bit.SYNCBUSY == 1);
}

void disableTimer(Tc *TMR){
	TcCount16* TC = (TcCount16*)TMR;
	
	TC->CTRLA.bit.ENABLE = 0;
	while (TC->STATUS.bit.SYNCBUSY == 1);
}

void setTimerFreq(Tc *TMR, uint16_t freq){
	
	TcCount16* TC = (TcCount16*)TMR;
	
	//set the frequency
	int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * freq)) - 1;
	// Make sure the count is in a proportional position to where it was
	// to prevent any jitter or disconnect when changing the compare value.
	TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
	TC->CC[0].reg = compareValue;
	while (TC->STATUS.bit.SYNCBUSY == 1);
}

void setupTimer(Tc *TMR){
	
	TcCount16* TC = (TcCount16*)TMR;
	
	TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;

	// Use the 16-bit timer
	TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
	while (TC->STATUS.bit.SYNCBUSY == 1);

	// Use match mode so that the timer counter resets when the count matches the compare register
	TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
	while (TC->STATUS.bit.SYNCBUSY == 1);

	// Set prescaler to 1024
	TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
	while (TC->STATUS.bit.SYNCBUSY == 1);

	// Enable the compare interrupt
	TC->INTENSET.reg = 0;
	TC->INTENSET.bit.MC0 = 1;
}

void configureTimers(){
	REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID (GCM_TCC2_TC3)) ;
	while ( GCLK->STATUS.bit.SYNCBUSY == 1 );
	
	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
	while (GCLK->STATUS.bit.SYNCBUSY);

	setupTimer(TC3);
	setupTimer(TC4);
	setupTimer(TC5);
}

void BspInit() {
	configureCTInt();
	configureTimers();
	
#ifdef ENABLE_BSP_PRINT
	SERIAL_OBJECT.begin(9600);
	char const *test = "BspInit success\n\r";
	BspWrite(test, strlen(test));
#endif // ENABLE_BSP_PRINT

	initialized = true;
}

void BspWrite(char const *buf, uint32_t len) {
	SERIAL_OBJECT.print(buf);
}

uint32_t GetSystemMs() {
	return millis();
}

extern "C" {
	int sysTickHook(void) {
		//don't run the sched until we've initialized
		if(initialized){
			QXK_ISR_ENTRY();
			QP::QF::tickX_(0);
			QXK_ISR_EXIT();
		}
		return 0;
	}
}

// namespace QP **************************************************************
namespace QP {

// QF callbacks ==============================================================
void QF::onStartup(void) {
	
    // assigning all priority bits for preemption-prio. and none to sub-prio.
    //NVIC_SetPriorityGrouping(0U);
	NVIC_DisableIRQ(EIC_IRQn);
	NVIC_ClearPendingIRQ(EIC_IRQn);
	
	NVIC_SetPriority(PendSV_IRQn, 0xFF);
	SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
	NVIC_SetPriority(SysTick_IRQn, SYSTICK_PRIO);
	NVIC_SetPriority(SERCOM5_IRQn, MIDI_UART_PRIO);
	NVIC_SetPriority(EIC_IRQn, EIC_PRIO);
	NVIC_SetPriority(TC3_IRQn, UPDATE_TMR_PRIO);
	NVIC_SetPriority(TC4_IRQn, LFO_TIMER_PRIO);
	NVIC_SetPriority(TC5_IRQn, LFO_TIMER2_PRIO);
	NVIC_SetPriority(USB_IRQn, USB_PRIO);
    
    // set priorities of ALL ISRs used in the system, see NOTE00
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
    // DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
    //NVIC_SetPriority(EXTI0_1_IRQn,   DPP::EXTI0_1_PRIO);
    // ...

    // enable IRQs...
    //NVIC_EnableIRQ(EXTI0_1_IRQn);
	NVIC_EnableIRQ(EIC_IRQn);
	NVIC_EnableIRQ(TC3_IRQn);
	NVIC_EnableIRQ(TC4_IRQn);
	NVIC_EnableIRQ(TC5_IRQn);
}

//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QXK::onIdle(void) {
    // toggle the User LED on and then off (not enough LEDs, see NOTE01)
    //QF_INT_DISABLE();
    //digitalWrite(13, HIGH);       // turn LED[n] on
    //digitalWrite(13, LOW);  // turn LED[n] off
    //QF_INT_ENABLE();


#if defined NDEBUG
    // Put the CPU and peripherals to the low-power mode.
    // you might need to customize the clock management for your application,
    // see the datasheet for your particular Cortex-M3 MCU.
    //
    // !!!CAUTION!!!
    // The WFI instruction stops the CPU clock, which unfortunately disables
    // the JTAG port, so the ST-Link debugger can no longer connect to the
    // board. For that reason, the call to __WFI() has to be used with CAUTION.
    //
    // NOTE: If you find your board "frozen" like this, strap BOOT0 to VDD and
    // reset the board, then connect with ST-Link Utilities and erase the part.
    // The trick with BOOT(0) is it gets the part to run the System Loader
    // instead of your broken code. When done disconnect BOOT0, and start over.
    //
    //__WFI();   Wait-For-Interrupt
#endif
}

//............................................................................
extern "C" void Q_onAssert(char const * const module, int loc) {
	//
    // NOTE: add here your application-specific error handling
    //

    QF_INT_DISABLE();
	SERIAL_OBJECT.print("**** ASSERT: ");
	SERIAL_OBJECT.print(module);
	SERIAL_OBJECT.print(" ");
	SERIAL_OBJECT.print(loc);
	SERIAL_OBJECT.println("****");
	while(1);
}

extern "C" void assert_failed(char const *module, int loc) {
	SERIAL_OBJECT.println("assert failed!");
	while(1);
}


} // namespace QP

