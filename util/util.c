/*
 * util.c
 *
 *  Created on: Aug 6, 2015
 *      Author: titan
 */

#include "util.h"
#include "../shared.h"
#include "task_monitor.h"

void reboot_1284(uint8_t cause)
{
	reset_cause.cause = cause;
	write_reset_cause();
	while(1);
}
