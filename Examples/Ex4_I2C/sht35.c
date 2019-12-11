/*
 * sht35.c
 *
 *  Created on: Apr 8, 2019
 *      Author: caleb
 */

#include "sht35.h"


void SHT35_sendCommand(uint8_t MSB, uint8_t LSB) {
	// Send command.
	EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B2_BASE, MSB);
	EUSCI_B_I2C_masterSendMultiByteFinish(EUSCI_B2_BASE, LSB);
	// Delay until transmission completes
	while (EUSCI_B_I2C_isBusBusy (EUSCI_B2_BASE))
		;
}

void SHT35_getTemp(uint8_t data[], uint8_t buffer[]) {
	uint32_t temp_raw = (data[0] << 8) + data[1];
	temp_raw *= 10000;
	int32_t temp = temp_raw / 65535;
	temp *= 315;
	temp -= 490000;
	temp /= 100;

	uint16_t number_length = sprintf((char*) buffer, "%d", temp);
	buffer[number_length + 1] = '\0';
	buffer[number_length] = buffer[number_length - 1];
	buffer[number_length - 1] = buffer[number_length - 2];
	buffer[number_length - 2] = '.';
}

void SHT35_getHumidity(uint8_t data[], uint8_t buffer[]) {
	uint32_t humidity_raw = (data[3] << 8) + data[4];
	humidity_raw *= 100000;
	uint32_t humidity = humidity_raw / 65535;
	humidity /= 10;

	uint16_t number_length = sprintf((char*) buffer, "%d", humidity);
	buffer[number_length + 1] = '\0';
	buffer[number_length] = buffer[number_length - 1];
	buffer[number_length - 1] = buffer[number_length - 2];
	buffer[number_length - 2] = '.';

}
