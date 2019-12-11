/*
 * esp32.c
 *
 *  Created on: Apr 20, 2019
 *      Author: Caleb
 */
#include "esp32.h"
#include "i2c/sht35.h"
#include "adc/adc.h"
#include <string.h>

extern uint8_t UART_buffer[];
extern uint8_t RXDATA[];
extern uint16_t ADC_A4_value;

void ESP32_ssid(uint8_t* ssid) {
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "AT+ssid=");
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, ssid);
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "\r");
}

void ESP32_pass(uint8_t* pass) {
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "AT+pass=");
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, pass);
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "\r");
}
void ESP32_connString(uint8_t* connString) {
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "AT+connString=");
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, connString);
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "\r");
}
void ESP32_telemetry(uint8_t* telemetry, uint8_t* value) {

	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "AT+telemetry=");
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, telemetry);
	EUSCI_A_UART_transmitData(EUSCI_A3_BASE, ',');
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, value);
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "\r");

}

void ESP32_mode(uint8_t mode) {
	EUSCI_A_UART_transmitString(EUSCI_A3_BASE, "AT+mode=");
	EUSCI_A_UART_transmitData(EUSCI_A3_BASE, mode);
	EUSCI_A_UART_transmitData(EUSCI_A3_BASE, '\r');

}

