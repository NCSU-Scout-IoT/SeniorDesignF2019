//*****************************************************************************
//! This example shows how to configure the I2C module as a master for
//! single byte reception in interrupt driven mode. The address of the slave
//! module that the master is communicating with also set in this example.
//!
//!  Description: This demo connects two MSP430's via the I2C bus. The master
//!  reads from the slave. This is the MASTER CODE. The data from the slave
//!  transmitter begins at 0 and increments with each transfer.
//!  The USCI_B0 RX interrupt is used to know when new data has been received.
//!  ACLK = n/a, MCLK = SMCLK = BRCLK =  DCO = 8MHz
//!
//!
//!                   SHT35                    MSP430FR5994
//!                   slave                       master
//!             -----------------            -----------------
//!            |              SDA|<-------->|P1.6/UCB0SDA  XIN|-
//!            |                 |          |                 | 32kHz
//!            |                 |          |             XOUT|-
//!            |              SCL|<-------->|P1.7/UCB0SCL     |
//!            |                 |          |             P1.0|--> LED
//!
//!
//
//*****************************************************************************
//*****************************************************************************
//
//Set the address for slave module. This is a 7-bit address sent in the
//following format:
//[A6:A5:A4:A3:A2:A1:A0:RS]
//
//A zero in the "RS" position of the first byte means that the master
//transmits (sends) data to the selected slave, and a one in this position
//means that the master receives data from the slave.
//
//*****************************************************************************

#include "driverlib.h"
#include "sht35.h"

#define SLAVE_ADDRESS 0x45

uint8_t RXData[12];
uint8_t count = 0;

void main(void) {
	//Stop WDT
	WDT_A_hold(WDT_A_BASE);

	//Set DCO frequency to 8MHz
	CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6);
	//Set SMCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
	//Set MCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	// Configure Pins for I2C
	//Set P7.0 and P7.1 as Primary Module Function Input.
	/*

	 * Select Port 7
	 * Set Pin 0, 1 to input Primary Module Function, (UCB2SIMO/UCB2SDA, UCB2SOMI/UCB2SCL).
	 */
	GPIO_setAsPeripheralModuleFunctionInputPin(
	GPIO_PORT_P7,
	GPIO_PIN0 + GPIO_PIN1,
	GPIO_PRIMARY_MODULE_FUNCTION);

	//Set P1.0 as an output pin.
	/*

	 * Select Port 1
	 * Set Pin 0 as output
	 */
	GPIO_setAsOutputPin(
	GPIO_PORT_P1,
	GPIO_PIN0);

	/*
	 * Disable the GPIO power-on default high-impedance mode to activate
	 * previously configured port settings
	 */
	PMM_unlockLPM5();

	EUSCI_B_I2C_initMasterParam param = { 0 };
	param.selectClockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
	param.i2cClk = CS_getSMCLK();
	param.dataRate = EUSCI_B_I2C_SET_DATA_RATE_400KBPS;
	param.byteCounterThreshold = 1;
	param.autoSTOPGeneration =
	EUSCI_B_I2C_NO_AUTO_STOP;
	EUSCI_B_I2C_initMaster(EUSCI_B2_BASE, &param);

	//Specify slave address
	EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE,
	SLAVE_ADDRESS);

	//Set Master in receive mode
	EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
	EUSCI_B_I2C_TRANSMIT_MODE);

	//Enable I2C Module to start operations
	EUSCI_B_I2C_enable(EUSCI_B2_BASE);

	EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
	EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
	EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);

	//Enable master Receive interrupt
	EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
	EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
	EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);

	__enable_interrupt();

	uint8_t temp[8];
	uint8_t humidity[8];
	while (1) {
		__delay_cycles(4000000);

		//Set Master in transmit mode
		EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
		EUSCI_B_I2C_TRANSMIT_MODE);

		// Send command to SHT35 to begin operation
		SHT35_sendCommand(FOUR_MPS, FOUR_HIGH_RP);

		__delay_cycles(2000000);

		//Set Master in transmit mode
		EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
		EUSCI_B_I2C_RECEIVE_MODE);

		count = 0;
		EUSCI_B_I2C_masterReceiveStart(EUSCI_B2_BASE);

		__delay_cycles(100);

		SHT35_getTemp(RXData, temp);
		SHT35_getHumidity(RXData, humidity);

	}
}

#pragma vector=USCI_B2_VECTOR
__interrupt
void USCIB2_ISR(void) {
	switch (__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG)) {
	case USCI_NONE:             // No interrupts break;
		break;
	case USCI_I2C_UCALIFG:      // Arbitration lost
		break;
	case USCI_I2C_UCNACKIFG:    // NAK received (master only)
		//EUSCI_B_I2C_masterReceiveStart(EUSCI_B0_BASE);
		break;
	case USCI_I2C_UCSTTIFG: // START condition detected with own address (slave mode only)
		break;
	case USCI_I2C_UCSTPIFG:     // STOP condition detected (master & slave mode)
		break;
	case USCI_I2C_UCRXIFG3:     // RXIFG3
		break;
	case USCI_I2C_UCTXIFG3:     // TXIFG3
		break;
	case USCI_I2C_UCRXIFG2:     // RXIFG2
		break;
	case USCI_I2C_UCTXIFG2:     // TXIFG2
		break;
	case USCI_I2C_UCRXIFG1:     // RXIFG1
		break;
	case USCI_I2C_UCTXIFG1:     // TXIFG1
		break;
	case USCI_I2C_UCRXIFG0:     // RXIFG0
		// Get RX data
		RXData[count] = EUSCI_B_I2C_masterReceiveSingle(EUSCI_B2_BASE);

		if (++count >= 5) {
			EUSCI_B_I2C_masterReceiveMultiByteStop(EUSCI_B2_BASE);
		}

		break; // Vector 24: RXIFG0 break;
	case USCI_I2C_UCTXIFG0:     // TXIFG0
		break;
	case USCI_I2C_UCBCNTIFG:    // Byte count limit reached (UCBxTBCNT)
		break;
	case USCI_I2C_UCCLTOIFG:    // Clock low timeout - clock held low too long
		break;
	case USCI_I2C_UCBIT9IFG: // Generated on 9th bit of a transmit (for debugging)
		break;
	default:
		break;
	}
}

