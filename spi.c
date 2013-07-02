/*
 * spi.c
 *
 * Created: 01.07.2013 14:00:44
 *  Author: d-wsky
 */ 

#ifndef  SPI_C
#define  SPI_C

#include "spi.h"
#include <avr/io.h>

void spi_init() {
	/* Set MOSI and SCK output, all others input */
	DDRB |= (1<<DD_MOSI)|(1<<DD_SCK);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SPI_DORD_MSB_TO_LSB;
}

///////////////////////////////////////////////////////////////////////////////
//                  SPI access                                               //
///////////////////////////////////////////////////////////////////////////////

/**************************************************         
Function: spi_read();                                         
                                                            
Description:                                                
	Writes one UINT8 to RFM73, and return the UINT8 read 
**************************************************/        
uint8_t spi_read(uint8_t value)                                    
{
	uint8_t res;                            
	/* Start transmission */
	SPDR = value;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF))) ;
	res = SPDR;
	return res;
}                                                           
                                                              
#endif