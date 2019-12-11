//******************************************************************************
//!  TIMER_A, Toggle P1.0, P4.0, CCR0 Cont. Mode ISR, DCO SMCLK
//!
//!  Toggle P1.0 using software and TA_0 ISR. Toggles every
//!  50000 SMCLK cycles. SMCLK provides clock source for TACLK.
//!  During the TA_0 ISR, P1.0 is toggled and 50000 clock cycles are added to
//!  CCR0. TA_0 ISR is triggered every 50000 cycles. CPU is normally off and
//!  used only during TA_ISR.
//!  ACLK = n/a, MCLK = SMCLK = TACLK = default DCO ~1.045MHz
//!
//!  Tested On: MSP430FR5994
//!         ---------------
//!     /|\|               |
//!      | |               |
//!      --|RST        P4.0|-->LED
//!        |               |
//!        |           P1.0|-->LED
//!
//!
//!
//
//*****************************************************************************
#include "driverlib.h"

#define COMPARE_VALUE 50000

void main(void) {
	//Stop WDT
	WDT_A_hold(WDT_A_BASE);

	// Setting up the clocks
	// Set DCO frequency to 1MHz
	CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_0);
	// Set SMCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
	// Set MCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	// Set P1.0 and P4.0 to output direction
	// These pins are connected to the two LEDs
	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN1);

	/*
	 * Disable the GPIO power-on default high-impedance mode to activate
	 * previously configured port settings
	 */
	PMM_unlockLPM5();

	// Start timer in continuous mode sourced by SMCLK
	Timer_A_initContinuousModeParam initContParam = { 0 };
	initContParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
	initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_8;
	initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
	initContParam.timerClear = TIMER_A_DO_CLEAR;
	initContParam.startTimer = false;
	Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam);

	// Initiaze compare mode
	Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);

	Timer_A_initCompareModeParam initCompParam = { 0 };
	initCompParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
	initCompParam.compareInterruptEnable =
	TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
	initCompParam.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
	// Initial compare value defined above
	initCompParam.compareValue = COMPARE_VALUE;
	Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);

	Timer_A_startCounter( TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);

	//Enter LPM0, enable interrupts
	__bis_SR_register(LPM0_bits + GIE);

	//For debugger
	__no_operation();
}

//******************************************************************************
//
//This is the TIMER1_A0 interrupt vector service routine.
//
//******************************************************************************
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER1_A0_ISR(void) {
	static bool toggleOtherLED = false;

	uint16_t compVal = Timer_A_getCaptureCompareCount(TIMER_A1_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0) + COMPARE_VALUE;

	//Toggle P1.0
	GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

	// Toggle P4.0 every other cycle
	if (toggleOtherLED) {
		GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN1);
	}
	toggleOtherLED = !toggleOtherLED;

	//Add Offset to CCR0
	Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0,
			compVal);
}

