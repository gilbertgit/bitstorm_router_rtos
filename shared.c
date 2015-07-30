/*
 * shared.c
 *
 *  Created on: Jul 22, 2015
 *      Author: titan
 */

#include <avr/eeprom.h>
#include "shared.h"

router_config_t EEMEM router_config_temp = { 12345678, 1234, 1 };

void read_config()
{

	eeprom_read_block(&router_config, &router_config_temp, sizeof(router_config_t));
}

void write_config()
{
	eeprom_update_block(&router_config, &router_config_temp, sizeof(router_config_t));
}
