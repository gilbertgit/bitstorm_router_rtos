/*
 * shared.c
 *
 *  Created on: Jul 22, 2015
 *      Author: titan
 */

#include <avr/eeprom.h>
#include "shared.h"

router_config_t EEMEM router_config_temp = { 12345678, 1234, 1 };

router_config_t EEMEM reset_cause_temp = { 12345678, 1234, 1 };

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

