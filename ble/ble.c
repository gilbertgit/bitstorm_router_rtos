/*
 * ble.c
 *
 *  Created on: Aug 6, 2015
 *      Author: titan
 */
#include <avr/io.h>
#include "ble.h"

void init_ble(void)
{
	DDRD |= BLE_CTS_bv;		// CTS - output
	DDRD &= ~_BV(PD4);		// RTS - input
	PORTD &= ~BLE_CTS_bv;		// lower CTS to start the BLE stream

	BLE_DDR |= BLE_EN_bv; 	// OUTPUT
	BLE_PORT &= ~BLE_EN_bv; // LOW
}

void kill_ble(void)
{
	BLE_PORT |= BLE_EN_bv; // HIGH
}
