/*
 * task_ble_monitor.c
 *
 *  Created on: Feb 23, 2015
 *      Author: titan
 */
#include "task_ble_monitor.h"
#include "task_ble_dispatch.h"
#include "../ramdisk/ramdisk.h"
#include "../util/clock.h"
#include "../wan/wan_task.h"

static btle_msg_t *next_msg = NULL;
static BaseType_t result;
static btle_msg_t *msg;

static portTASK_FUNCTION( task_ble_monitor, params)
{
	for (;;)
	{
		if (next_msg != NULL )
		{
			msg = next_msg;
			next_msg = ramdisk_next(next_msg);
			if ((clock_time() - msg->last_sent) >= 10000)
			{
				//led_alert_on();
				//send "is out of prox packet"
				msg->type = MSG_TYPE_OUT_PROX;
				result = xQueueSendToBack( xWANQueue, msg, 0);
				// erase the packet
				ramdisk_erase(*msg);
			}

		} else
		{
			next_msg = ramdisk_next(NULL );
		}
	}
}

void task_ble_monitor_start(UBaseType_t uxPriority)
{

	xTaskCreate(task_ble_monitor, "blemonitor", configMINIMAL_STACK_SIZE, NULL,
			uxPriority, (TaskHandle_t *) NULL);
}
