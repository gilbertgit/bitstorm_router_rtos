/*
 * wan.h
 *
 *  Created on: May 21, 2015
 *      Author: titan
 */

#ifndef WAN_H_
#define WAN_H_

#define WAN_DDR		DDRB
#define WAN_PORT	PORTB
#define WAN_CTS_bv		(_BV(PB0))
#define WAN_CONFIG_bv	(_BV(PB1))
#define WAN_EN_bv		(_BV(PB3))
#define WAN_READY_ISR_MSK	(1 << PCINT8)
#define WAN_CONFIG_ISR_MSK	(1 << PCINT9)

void init_wan(void);
void kill_wan(void);


#endif /* WAN_H_ */
