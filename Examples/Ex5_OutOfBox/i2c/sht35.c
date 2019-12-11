/*
 * sht35.c
 *
 *  Created on: Apr 8, 2019
 *      Author: caleb
 */

#include "sht35.h"

uint8_t sht35data[6];

void SHT35_sendCommand(uint8_t MSB, uint8_t LSB) {
	// Send command.
	EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B2_BASE, MSB);
	EUSCI_B_I2C_masterSendMultiByteFinish(EUSCI_B2_BASE, LSB);
	// Delay until transmission completes
	while (EUSCI_B_I2C_isBusBusy(EUSCI_B2_BASE))
		;
}

void SHT35_getTemp(uint8_t data[], uint8_t temp_string[]) {
	uint32_t temp_raw = (data[0] << 8) + data[1];
	temp_raw *= 10000;
	int32_t temp = temp_raw / 65535;
	temp *= 315;
	temp -= 490000;
	temp /= 100;

	uint16_t number_length = sprintf((char*) temp_string, "%d", temp);
	temp_string[number_length + 1] = '\0';
	temp_string[number_length] = temp_string[number_length - 1];
	temp_string[number_length - 1] = temp_string[number_length - 2];
	temp_string[number_length - 2] = '.';
}

void SHT35_getHumidity(uint8_t data[], uint8_t humidity_string[]) {
	uint32_t humidity_raw = (data[3] << 8) + data[4];
	humidity_raw *= 10000;
	uint32_t humidity = humidity_raw / 65535;
	humidity /= 10;

	uint16_t number_length = sprintf((char*) humidity_string, "%d", humidity);
	humidity_string[number_length + 1] = '\0';
	humidity_string[number_length] = humidity_string[number_length - 1];
	humidity_string[number_length - 1] = humidity_string[number_length - 2];
	humidity_string[number_length - 2] = '.';

}
