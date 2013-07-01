/*
 * uart.h
 *
 * Created: 15.06.2013 13:38:11
 *  Author: d-wsky
 */ 


#ifndef UART_H_
#define UART_H_


#include <avr/io.h>
#include <stdio.h>

#ifndef F_CPU
	#define F_CPU  14835100UL
#endif

int uart_putchar(char c, FILE *stream);
void uart_init(unsigned char port, unsigned int baudrate);


#endif /* UART_H_ */