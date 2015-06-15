/*
 * spi.c
 *
 *  Created on: May 18, 2015
 *      Author: titan
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include <avr/io.h>
#include "spi.h"

#define WAN_DDR		DDRB
#define WAN_PORT	PORTB
#define WAN_SS		(1 << PB4)
#define WAN_MOSI	(1 << PB5)
#define WAN_SCK		(1 << PB7)

uint8_t send_spi_msg;

void init_spi_master()
{
	//set MOSI, SCK and SS as output
	WAN_DDR |= WAN_MOSI | WAN_SCK | WAN_SS;
	//set SS to high
	WAN_PORT |= (1 << PB4);
	//enable master SPI at clock rate Fck/4
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

	send_spi_msg = 0x01;
}

void spi_ss_on()
{
	//select slave - low
	WAN_PORT &= ~WAN_SS;
}

void spi_ss_off()
{
	// SS to high
	WAN_PORT |= WAN_SS;
}

void spi_dumby_byte()
{
	SPDR = 0xFF;
	//wait for transmition complete
	while (!(SPSR & (1 << SPIF)))
		;
}

//Function to send and receive data for both master and slave
unsigned char spi_tranceiver(unsigned char data)
{
	//send data
	SPDR = data;
	//wait for transmition complete
	while (!(SPSR & (1 << SPIF)))
		;

	return SPDR ;
}
