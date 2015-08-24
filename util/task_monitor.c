/*
 * task_monitor.c
 *
 *  Created on: Mar 26, 2015
 *      Author: titan
 */

#include "task_monitor.h"
#include "../wan/wan_task.h"
#include "../wan/task_wan_dispatch.h"
#include "wan.h"
#include "../ble/task_ble_serial.h"
#include "util.h"
#include "../shared.h"

#define COUNTER_MAX 5

static TaskHandle_t xTaskMonitorHandle;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

static uint16_t wan_previous_counter = 0;
static uint8_t wan_zero_counter = 0;

static uint16_t ble_previous_counter = 0;
static uint8_t ble_zero_counter = 0;

static uint16_t ble_dispatch_previous_counter = 0;
static uint8_t ble_dispatch_zero_counter = 0;

static uint16_t wan_dispatch_previous_counter = 0;
static uint8_t wan_dispatch_zero_counter = 0;

reset_cause_t reset_cause;

static portTASK_FUNCTION(task_monitor, params)
{

	// LED #1 ///////////////
	DDRA |= _BV(PA0);
	PORTA |= _BV(PA0);
	////////////////////////
	read_reset_cause();
	for (;;)
	{
		wdt_reset();
		vTaskDelay(xDelay); // do the check every half second // A task would have to hang 2.5 seconds before a reset is triggered
		PORTA ^= _BV(PA0);
		wan_task_monitor();
		ble_task_monitor();
		ble_dispatch_task_monitor();
	}
}

void wan_task_monitor()
{
	if ((xWanMonitorCounter - wan_previous_counter) == 0)
	{
		wan_zero_counter++;

		if (wan_zero_counter >= COUNTER_MAX)
		{
			vTaskDelay(xDelay);

			// we got problems, log reset cause, REBOOT
			reboot_1284(WAN_TASK_M);
		}
	} else
	{
		wan_zero_counter = 0;
	}

	wan_previous_counter = xWanMonitorCounter;
}

void ble_task_monitor()
{
	if ((xBleMonitorCounter - ble_previous_counter) == 0)
	{
		ble_zero_counter++;

		if (ble_previous_counter >= COUNTER_MAX)
		{
			vTaskDelay(xDelay);

			// we got problems, log reset cause, REBOOT
			reboot_1284(BLE_TASK_M);
		}
	} else
	{
		ble_zero_counter = 0;
	}

	ble_previous_counter = xBleMonitorCounter;
}

void ble_dispatch_task_monitor()
{
	if ((xBleDispatchMonitorCounter - ble_dispatch_previous_counter) == 0)
	{
		ble_dispatch_zero_counter++;

		if (ble_dispatch_previous_counter >= COUNTER_MAX)
		{
			vTaskDelay(xDelay);

			// we got problems, log reset cause, REBOOT
			reboot_1284(BLE_DISPATCH_TASK_M);
		}
	} else
	{
		ble_dispatch_zero_counter = 0;
	}

	ble_dispatch_previous_counter = xBleDispatchMonitorCounter;
}

void wan_dispatch_task_monitor()
{
	if ((xWanDispatchMonitorCounter - wan_dispatch_previous_counter) == 0)
	{
		wan_dispatch_zero_counter++;

		if (wan_dispatch_previous_counter >= COUNTER_MAX)
		{
			vTaskDelay(xDelay);

			// we got problems, log reset cause, REBOOT
			reboot_1284(WAN_DISPATCH_TASK_M);
		}
	} else
	{
		wan_dispatch_zero_counter = 0;
	}

	wan_dispatch_previous_counter = xWanDispatchMonitorCounter;
}

void task_monitor_start(UBaseType_t uxPriority)
{
	xTaskCreate(task_monitor, "taskmonitor", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xTaskMonitorHandle);
}
