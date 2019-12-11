//
#include "driverlib.h"
#include "i2c/sensor_i2c.h"
#include "i2c/sht35.h"
#include "timers.h"
#include "ports.h"
#include "uart/uart.h"
#include "uart/esp32.h"
#include "adc/adc.h"

#define ADC_A3
#define ADC_A4
#define ADC

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

extern bool client_connected;
extern uint8_t RXDATA[];
extern uint16_t ADC_A3_value;
extern uint16_t ADC_A4_value;
extern bool ok;

void main(void) {
	WDT_A_hold(WDT_A_BASE);

	//Set DCO frequency to 8MHz
	CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6);
	//Set ACLK = VLO with frequency divider of 1
	CS_initClockSignal(CS_ACLK, CS_VLOCLK_SELECT, CS_CLOCK_DIVIDER_1);
	//Set SMCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
	//Set MCLK = DCO with frequency divider of 1
	CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	init_ports();

	/*
	 * Disable the GPIO power-on default high-impedance mode to activate
	 * previously configured port settings
	 */
	PMM_unlockLPM5();
#ifdef ADC
	init_ADC12B();

	init_ADC12B_memoryBuffer(ADC12_B_MEMORY_0, ADC12_B_INPUT_A3,
	ADC12_B_NOTENDOFSEQUENCE, ADC12_B_IFG0,
	ADC12_B_IE0);

	init_ADC12B_memoryBuffer(ADC12_B_MEMORY_0, ADC12_B_INPUT_A4,
	ADC12_B_ENDOFSEQUENCE, ADC12_B_IFG1,
	ADC12_B_IE1);

#endif

	UART_init(EUSCI_A0_BASE);
	UART_init(EUSCI_A3_BASE);

	timer_a_init(TIMER_A0_BASE);
	__enable_interrupt();
	// Enable ESp32
	ESP32_mode('0');
	//while(!ok);
	ESP32_ssid("ClickForFreeViruses-2.4G");
	//while(!ok);
	ESP32_pass("u0y8-lokv-bu9x");
	//while(!ok);
	ESP32_connString(
			"HostName=iothub-mhvvc.azure-devices.net;DeviceId=63260816-6df9-4dae-8b87-afa2816fab8f;SharedAccessKey=uw3SedOYkJVwkIxTVEivzWNwMKaPxMjzBZcirOPtz+Y=");
	//while(!ok);

#ifdef I2C
	I2C_init();
#endif
#ifdef SHT35
	SHT35_sendCommand(FOUR_MPS, FOUR_HIGH_RP);
#endif
	while (1) {

		//Delay between each transaction
		__bis_SR_register(LPM1_bits + GIE);
#ifdef I2C
		I2C_initReceive();
		uint8_t temp[16];
		uint8_t humidity[16];
		if (RXDATA[0] != '\0') {
			SHT35_getTemp(RXDATA, temp);
			ESP32_telemetry("temperature", temp);
			SHT35_getHumidity(RXDATA, humidity);
			ESP32_telemetry("humidity", humidity);
		}
#endif
#ifdef ADC_A3
		ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_MEMORY_0,
		ADC12_B_SEQOFCHANNELS);
		uint8_t moisture[16];
		ADC_getPercentage(moisture, ADC_A3_value, 1100);
		ESP32_telemetry("moisture", moisture);

#endif
#ifdef ADC_A4
		uint8_t light[16];
		ADC_getPercentage(light, ADC_A4_value, 3000);
		ESP32_telemetry("light", light);
#endif

	}
}

