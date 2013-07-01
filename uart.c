/*
 * uart.c
 *
 * Created: 15.06.2013 13:37:19
 *  Author: d-wsky
 */ 
#include "uart.h"

int uart_putchar(char c, FILE *stream) {
    if (c == '\n')
    uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 0;
}

void uart_init(unsigned char port, unsigned int baudrate) {
	uint16_t ubrr = F_CPU/16/baudrate-1;
	if (port==1) {
		UBRR1H=(unsigned char)(ubrr>>8);
		UBRR1L=(unsigned char) ubrr;
		UCSR1A=0x00;
		/* Разрешение работы передатчика и приемника */
		UCSR1B=(1<<RXEN1)|(1<<TXEN1);
		/* Установка формата посылки: 8 бит данных, 1 стоп-бит */
		UCSR1C=(1<<UCSZ11)|(1<<UCSZ10);
	}
	else {
		UBRR0H=(unsigned char)(ubrr>>8);
		UBRR0L=(unsigned char) ubrr;
		UCSR0A=0x00;
		/* Разрешение работы передатчика и приемника */
		UCSR0B=(1<<RXEN0)|(1<<TXEN0);
		/* Установка формата посылки: 8 бит данных, 1 стоп-бит */
		UCSR0C=(1<<UCSZ01)|(1<<UCSZ00);
	};	
}

