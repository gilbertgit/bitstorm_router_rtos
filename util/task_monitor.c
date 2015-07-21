/*
 * task_monitor.c
 *
 *  Created on: Mar 26, 2015
 *      Author: titan
 */

#include "task_monitor.h"
#include "../wan/wan_task.h"
#include "wan.h"

#define COUNTER_MAX 5

static TaskHandle_t xTaskMonitorHandle;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

static uint16_t wan_previous_counter = 0;
static uint8_t wan_zero_counter = 0;

void wan_task_monitor();

static portTASK_FUNCTION(task_monitor, params)
{
	for (;;)
	{
		wdt_reset();
		vTaskDelay(xDelay); // do the check every half second
		wan_task_monitor();
	}
}

void wan_task_monitor()
{
	if ((xWanMonitorCounter - wan_previous_counter) == 0)
	{
		wan_zero_counter++;

		if (wan_zero_counter >= COUNTER_MAX)
		{
//			led_alert_on();
			vTaskDelay(xDelay);

			// we got problems, REBOOT
			kill_wan();

			reboot_1284();
		}
	}

	wan_previous_counter = xWanMonitorCounter;
}

void reboot_1284()
{
	while(1);
}

void task_monitor_start(UBaseType_t uxPriority)
{
	xTaskCreate(task_monitor, "taskmonitor", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xTaskMonitorHandle);
}
