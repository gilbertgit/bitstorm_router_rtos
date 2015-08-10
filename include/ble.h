/*
 * ble.h
 *
 *  Created on: Aug 6, 2015
 *      Author: titan
 */

#ifndef BLE_H_
#define BLE_H_

#define BLE_DDR		DDRC
#define BLE_PORT	PORTC
#define BLE_CTS_bv	(_BV(PD5))
#define BLE_RTS_bv	(_BV(PD4))
#define BLE_EN_bv	(_BV(PC7))
//#define BLE_READY_ISR_MSK	(1 << PCINT8)
//#define BLE_CONFIG_ISR_MSK	(1 << PCINT9)

void init_ble(void);
void kill_ble(void);

#endif /* BLE_H_ */
