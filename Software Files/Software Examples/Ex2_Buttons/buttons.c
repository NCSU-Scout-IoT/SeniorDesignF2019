//******************************************************************************
//   Software Port Interrupt Service on P4.1 from LPM4 with
//                     Internal Pull-up Resistance Enabled
//
//   A hi "TO" low transition on P4.1 (S1) or P4.2 (S2) will trigger
//   P5_ISR which toggles P1.0. P1.1 is internally enabled to pull-up. Normal mode is
//   LPM4 ~ 0.1uA. LPM4 current can be measured with the LED removed, all
//   unused Px.x configured as output or inputs pulled high or low. On the
//   MSP430FR5969 LaunchPad, P1.1 is connected to button S1. The interrupt is
//   triggered once the switch is released.
//   ACLK = n/a, MCLK = SMCLK = default DCO
//
//  Tested On: MSP430FR5994
//             -----------------
//            |                 |
//            |                 |
//         -->|P4.1         P4.0|-->LED
//            |                 |
//         -->|P4.2         P1.0|-->LED
//
//
//   This example uses the following peripherals and I/O signals.  You must
//   review these and change as needed for your own board:
//   - GPIO Port peripheral
//
//   This example uses the following interrupt handlers.  To use this example
//   in your own application you must add these interrupt handlers to your
//   vector table.
//   - PORT5_VECTOR
//******************************************************************************
#include "driverlib.h"

void main(void) {
	//Stop watchdog timer
	WDT_A_hold(WDT_A_BASE);

	//Set P1.0 to output direction
	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

	// Set P4.0 to output direction
	GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0);

	//Enable P4.1 and P4.2 internal resistance as pull-Up resistance
	GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2);

	//P4.1 and P4.2 Hi/Lo edge
	GPIO_selectInterruptEdge(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2,
			GPIO_HIGH_TO_LOW_TRANSITION);

	/*
	 * Disable the GPIO power-on default high-impedance mode to activate
	 * previously configured port settings
	 */
	PMM_unlockLPM5();

	//P4.1 & P4.2 IFG cleared
	GPIO_clearInterrupt( GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2);

	//P4.1 & P4.2 interrupt enabled
	GPIO_enableInterrupt( GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2);

	//Enter LPM4 w/interrupt
	__bis_SR_register(LPM4_bits + GIE);

	//For debugger
	__no_operation();
}

//******************************************************************************
//
//This is the PORT5_VECTOR interrupt vector service routine
//
//******************************************************************************
#pragma vector=PORT4_VECTOR
__interrupt
void Port_4(void) {
	static uint8_t toggleOtherLED = 0;
	switch (__even_in_range(P4IV, 16)) {
	case 4:
		//P1.0 = toggle
		GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
		// P4.0 toggle if P1.0 is low
		if(toggleOtherLED){
			GPIO_toggleOutputOnPin(GPIO_PORT_P4, GPIO_PIN0);
		}
		toggleOtherLED = !toggleOtherLED;
		break;
	case 6:
		// Set both outputs low
		GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0);
		break;
	}
	//P4.1 and P4.2 IFG cleared
	GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2);
}
