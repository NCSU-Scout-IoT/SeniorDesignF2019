//******************************************************************************
//  MSP430FR5994 Demo - ADC12B, Sample A1, AVcc Ref, Set P1.0 if A1 > 0.5*AVcc
//              Converts 12-bit integer into string representation of percentage
//
//   Description: A single sample is made on A1 with reference to AVcc.
//   Software sets ADC12BSC to start sample and conversion - ADC12BSC
//   automatically cleared at EOC. ADC12B internal oscillator times sample (16x)
//   and conversion. If A0 > 0.5*AVcc, P1.0 set, else reset. The full, correct handling of
//   and ADC12B interrupt is shown as well. A string representation of the
//	 percentage out of 2^12 is constructed.
//
//
//                MSP430FR5969
//             -----------------
//         /|\|                 |
//          | |                 |
//          --|RST              |
//            |                 |
//        >---|P1.4/A4      P1.0|-->LED
//
//******************************************************************************
#include "driverlib.h"
#include <stdio.h>

#define MAXIMUM (1000)

// Global variable
uint16_t ADC_value;

// Function declaration
void ADC_getPercentage(uint8_t buffer[], uint16_t value, uint16_t maximum);

void main(void) {
	// Stop WDT
	WDT_A_hold(WDT_A_BASE);

	//Set P1.0 as an output pin.
	/*

	 * Select Port 1
	 * Set Pin 0 as output
	 */
	GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN0);

	//Set P1.0 as Output Low.
	/*

	 * Select Port 1
	 * Set Pin 0 to output Low.
	 */
	GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0);
	//Set P1.4 as Ternary Module Function Output.
	/*

	 * Select Port 1
	 * Set Pin 4 to output Ternary Module Function, (A4, C4, VREF+, VeREF+).
	 */
	GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN4,
	GPIO_TERNARY_MODULE_FUNCTION);

	/*
	 * Disable the GPIO power-on default high-impedance mode to activate
	 * previously configured port settings
	 */
	PMM_unlockLPM5();

	//Initialize the ADC12B Module
	/*
	 * Base address of ADC12B Module
	 * Use internal ADC12B bit as sample/hold signal to start conversion
	 * USE MODOSC 5MHZ Digital Oscillator as clock source
	 * Use default clock divider/pre-divider of 1
	 * Not use internal channel
	 */
	ADC12_B_initParam initParam = { 0 };
	initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC;
	initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ADC12OSC;
	initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
	initParam.clockSourcePredivider = ADC12_B_CLOCKPREDIVIDER__1;
	initParam.internalChannelMap = ADC12_B_NOINTCH;
	ADC12_B_init(ADC12_B_BASE, &initParam);

	//Enable the ADC12B module
	ADC12_B_enable(ADC12_B_BASE);

	/*
	 * Base address of ADC12B Module
	 * For memory buffers 0-7 sample/hold for 64 clock cycles
	 * For memory buffers 8-15 sample/hold for 4 clock cycles (default)
	 * Disable Multiple Sampling
	 */
	ADC12_B_setupSamplingTimer(ADC12_B_BASE, ADC12_B_CYCLEHOLD_16_CYCLES,
			ADC12_B_CYCLEHOLD_4_CYCLES, ADC12_B_MULTIPLESAMPLESDISABLE);

	//Configure Memory Buffer
	/*
	 * Base address of the ADC12B Module
	 * Configure memory buffer 0
	 * Map input A1 to memory buffer 0
	 * Vref+ = AVcc
	 * Vref- = AVss
	 * Memory buffer 0 is not the end of a sequence
	 */
	ADC12_B_configureMemoryParam configureMemoryParam = { 0 };
	configureMemoryParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
	configureMemoryParam.inputSourceSelect = ADC12_B_INPUT_A4;
	configureMemoryParam.refVoltageSourceSelect =
			ADC12_B_VREFPOS_AVCC_VREFNEG_VSS;
	configureMemoryParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;
	configureMemoryParam.windowComparatorSelect =
			ADC12_B_WINDOW_COMPARATOR_DISABLE;
	configureMemoryParam.differentialModeSelect =
			ADC12_B_DIFFERENTIAL_MODE_DISABLE;
	ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParam);

	ADC12_B_clearInterrupt(ADC12_B_BASE, 0, ADC12_B_IFG0);

	//Enable memory buffer 0 interrupt
	ADC12_B_enableInterrupt(ADC12_B_BASE, ADC12_B_IE0, 0, 0);
	__enable_interrupt();

	while (1) {
		__delay_cycles(5000);

		//Enable/Start sampling and conversion
		/*
		 * Base address of ADC12B Module
		 * Start the conversion into memory buffer 0
		 * Use the single-channel, single-conversion mode
		 */
		ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_MEMORY_0,
		ADC12_B_SINGLECHANNEL);
		uint8_t percentage[8];
		ADC_getPercentage(percentage, ADC_value, 1000);

	}
}

#pragma vector=ADC12_VECTOR
__interrupt
void ADC12_ISR(void) {
	switch (__even_in_range(ADC12IV, 12)) {
	case 0:
		break;                         // Vector  0:  No interrupt
	case 2:
		break;                         // Vector  2:  ADC12BMEMx Overflow
	case 4:
		break;                         // Vector  4:  Conversion time overflow
	case 6:
		break;                         // Vector  6:  ADC12BHI
	case 8:
		break;                         // Vector  8:  ADC12BLO
	case 10:
		break;                         // Vector 10:  ADC12BIN
	case 12:                                // Vector 12:  ADC12BMEM0 Interrupt
		ADC_value = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_0);
		if (ADC_value >= (MAXIMUM/2) ) {
			//Set P1.0 LED on
			/*

			 * Select Port 1
			 * Set Pin 0 to output high.
			 */
			GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN0);
		} else {
			//Set P1.0 LED off
			/*

			 * Select Port 1
			 * Set Pin 0 to output high.
			 */
			GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0);
		}
		break;
	case 14:
		break;                         // Vector 14:  ADC12BMEM1
	case 16:
		break;                         // Vector 16:  ADC12BMEM2
	case 18:
		break;                         // Vector 18:  ADC12BMEM3
	case 20:
		break;                         // Vector 20:  ADC12BMEM4
	case 22:
		break;                         // Vector 22:  ADC12BMEM5
	case 24:
		break;                         // Vector 24:  ADC12BMEM6
	case 26:
		break;                         // Vector 26:  ADC12BMEM7
	case 28:
		break;                         // Vector 28:  ADC12BMEM8
	case 30:
		break;                         // Vector 30:  ADC12BMEM9
	case 32:
		break;                         // Vector 32:  ADC12BMEM10
	case 34:
		break;                         // Vector 34:  ADC12BMEM11
	case 36:
		break;                         // Vector 36:  ADC12BMEM12
	case 38:
		break;                         // Vector 38:  ADC12BMEM13
	case 40:
		break;                         // Vector 40:  ADC12BMEM14
	case 42:
		break;                         // Vector 42:  ADC12BMEM15
	case 44:
		break;                         // Vector 44:  ADC12BMEM16
	case 46:
		break;                         // Vector 46:  ADC12BMEM17
	case 48:
		break;                         // Vector 48:  ADC12BMEM18
	case 50:
		break;                         // Vector 50:  ADC12BMEM19
	case 52:
		break;                         // Vector 52:  ADC12BMEM20
	case 54:
		break;                         // Vector 54:  ADC12BMEM21
	case 56:
		break;                         // Vector 56:  ADC12BMEM22
	case 58:
		break;                         // Vector 58:  ADC12BMEM23
	case 60:
		break;                         // Vector 60:  ADC12BMEM24
	case 62:
		break;                         // Vector 62:  ADC12BMEM25
	case 64:
		break;                         // Vector 64:  ADC12BMEM26
	case 66:
		break;                         // Vector 66:  ADC12BMEM27
	case 68:
		break;                         // Vector 68:  ADC12BMEM28
	case 70:
		break;                         // Vector 70:  ADC12BMEM29
	case 72:
		break;                         // Vector 72:  ADC12BMEM30
	case 74:
		break;                         // Vector 74:  ADC12BMEM31
	case 76:
		break;                         // Vector 76:  ADC12BRDY
	default:
		break;
	}
}

void ADC_getPercentage(uint8_t buffer[], uint16_t value, uint16_t maximum) {
	// convert to 32 bit integer and multiply by large number
	uint32_t long_value = ((uint32_t) value) * 100000;
	// divide by maximum to get "percentage"
	uint32_t percentage = long_value / maximum;
	// divide to get only two decimal places
	percentage /= 10;

	// use sprintf to convert to string form
	uint16_t number_length = sprintf((char*) buffer, "%d", percentage);
	// inserting the decimal point
	uint16_t i;
	int16_t j;
	// this loop pads zeros to the beginning of the buffer to guarantee
	// three digits before the decimal point is added
	for (i = number_length; i < 3; i++) {
		// move characters over by one
		for (j = number_length; j >= 0; j--) {
			buffer[j + 1] = buffer[j];
		}
		// add the zero to the start
		buffer[0] = '0';
	}
	// insure terminal null at end of string
	buffer[i + 1] = '\0';
	// move over decimal places by one
	buffer[i] = buffer[i - 1];
	buffer[i - 1] = buffer[i - 2];
	// insert decimal point
	buffer[i - 2] = '.';
}
