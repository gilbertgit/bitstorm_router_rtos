/*
 * task_monitor.c
 *
 *  Created on: Mar 26, 2015
 *      Author: titan
 */

#include "task_monitor.h"
#include "../wan/wan_task.h"
#include "wan.h"
#include "../ble/task_ble_serial.h"
#include "util.h"

#define COUNTER_MAX 5

static TaskHandle_t xTaskMonitorHandle;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

static uint16_t wan_previous_counter = 0;
static uint8_t wan_zero_counter = 0;

static uint16_t ble_previous_counter = 0;
static uint8_t ble_zero_counter = 0;

static uint16_t ble_dispatch_previous_counter = 0;
static uint8_t ble_dispatch_zero_counter = 0;

static portTASK_FUNCTION(task_monitor, params)
{
	// LED #1 ///////////////
	DDRA |= _BV(PA0);
	PORTA |= _BV(PA0);
	////////////////////////

	init_wd();
	for (;;)
	{
		wdt_reset();
		vTaskDelay(xDelay); // do the check every half second
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
//			led_alert_on();
			vTaskDelay(xDelay);

			// we got problems, REBOOT
			//kill_wan();

			reboot_1284();
		}
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

			// we got problems, REBOOT
			//kill_wan();

			reboot_1284();
		}
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

			// we got problems, REBOOT
			//kill_wan();

			reboot_1284();
		}
	}

	ble_dispatch_previous_counter = xBleDispatchMonitorCounter;
}

void init_wd()
{
	wdt_disable();
	WDTCSR |= (1 << WDCE) | (1 << WDE); // reset mode
	WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0); // 64k Timeout
	wdt_enable(WDTO_2S);
}


void task_monitor_start(UBaseType_t uxPriority)
{
	xTaskCreate(task_monitor, "taskmonitor", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xTaskMonitorHandle);
}
