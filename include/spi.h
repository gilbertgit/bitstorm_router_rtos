/*
 * spi.h
 *
 *  Created on: May 18, 2015
 *      Author: titan
 */

#ifndef SPI_H_
#define SPI_H_

extern uint8_t send_spi_msg;

void init_spi_master();
unsigned char spi_tranceiver (unsigned char data);

void spi_ss_on();
void spi_ss_off();

void spi_dumby_byte();


#endif /* SPI_H_ */
