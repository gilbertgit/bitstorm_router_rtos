/*
 * shared.c
 *
 *  Created on: Jul 22, 2015
 *      Author: titan
 */

#include <avr/eeprom.h>
#include "shared.h"

//typedef struct
//{
//	uint8_t magic;
//	uint64_t mac;
//	uint16_t pan_id;
//	uint8_t channel;
//}router_config_t;


router_config_t EEMEM router_config_temp = { 0xFF, 0x1122334455667788, 0x1122, 0x11 };

reset_cause_t EEMEM reset_cause_temp = { 0xFF, 0xFFFF };

rssi_threshold_t EEMEM rssi_threshold_temp = { 0xFF };

void read_config()
{

	eeprom_read_block(&router_config, &router_config_temp, sizeof(router_config_t));
}

void write_config()
{
	eeprom_update_block(&router_config, &router_config_temp, sizeof(router_config_t));
}


void read_reset_cause()
{
	eeprom_read_block(&reset_cause, &reset_cause_temp, sizeof(reset_cause_t));
}

void write_reset_cause()
{
	eeprom_update_block(&reset_cause, &reset_cause_temp, sizeof(reset_cause_t));
}

void read_rssi_threshold()
{
	eeprom_read_block(&rssi_threshold, &rssi_threshold_temp, sizeof(rssi_threshold_t));
}

void write_rssi_threshold()
{
	eeprom_update_block(&rssi_threshold, &rssi_threshold_temp, sizeof(rssi_threshold_t));
}

