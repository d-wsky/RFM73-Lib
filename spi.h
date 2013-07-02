/*
 * spi.h
 *
 * Created: 01.07.2013 14:00:55
 *  Author: d-wsky
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <inttypes.h>

#define DD_MOSI           PB2
#define DD_SCK            PB1

#define SPI_DORD_LSB_TO_MSB   SPCR |= (1 << DORD)
#define SPI_DORD_MSB_TO_LSB   SPCR &=~(1 << DORD)

extern void spi_init();
extern uint8_t spi_read(uint8_t value);

#endif /* SPI_H_ */