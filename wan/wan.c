/*
 * wan.c
 *
 *  Created on: May 21, 2015
 *      Author: titan
 */

#include <avr/io.h>
#include "wan.h"

#define WAN_DDR		DDRB
#define WAN_PORT	PORTB
#define WAN_CTS_bv		(_BV(PB0))
#define WAN_CONFIG_bv	(_BV(PB1))
#define WAN_EN_bv		(_BV(PB3))
#define WAN_ISR_MSK	(1 << PCINT8)

void init_wan(void)
{
	//PB0 is used for CTS of message
	WAN_DDR &= ~WAN_CTS_bv;	// SET PB0 TO INPUT

	// set up interrupt
	PCICR |= (1 << PCIE1);
	PCMSK1 |= WAN_ISR_MSK;

	//PB1 is used for configure
	WAN_DDR &= ~WAN_CONFIG_bv; //INPUT

	// Enable WAN - Turn the ZB on
	WAN_DDR |= WAN_EN_bv; // OUTPUT
	PORTB &= ~WAN_EN_bv; // LOW
}

void kill_wan(void)
{
	// Make everything on PORTB low
	WAN_DDR = 0xFF;

	// disable the interrupts
	PCMSK1 &= ~WAN_ISR_MSK;

	// disable WAN - Turn the ZB off
	WAN_PORT |= WAN_EN_bv; // HIGH
}
