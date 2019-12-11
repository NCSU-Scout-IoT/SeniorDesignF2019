/*
 * ports.c
 *
 *  Created on: Apr 20, 2019
 *      Author: Caleb
 */

#include "ports.h"

void init_ports(void){
    I2C_initPorts();
    UART_initPorts();
    ADC_initPorts();
    init_port8();
}


void init_port8(void){
    P8SEL0 &= ~0x2;
    P8SEL1 &= ~0x2;
    P8DIR |= 0x2;
    P8OUT &= ~0x2;
}
