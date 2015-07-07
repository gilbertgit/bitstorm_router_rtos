/*
 * task_wan_dispatch.c
 *
 *  Created on: Jul 6, 2015
 *      Author: titan
 */
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"
#include "task_wan_dispatch.h"
#include "wan_task.h"

#define NWK_READY (PINB & (1 << PB0))
#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10

enum comands {
	CONFIG_RESP = 0x03, CHANGESET = 0X09
};

static signed char outBuffer[BUFFER_SIZE];
static BaseType_t result;
bool queue_created = false;

QueueHandle_t xWanDispatchQueue;
TaskHandle_t xWanDispatchHandle;
static xComPortHandle pxWan;


static portTASK_FUNCTION(task_wan_dispatch, params)
{
	pxWan = xSerialPortInitMinimal(0, 38400, 50);

	uint8_t cmd;
	bool has_syncd;

	for (;;)
	{
		has_syncd = false;
		result = xQueueReceive( xWanDispatchQueue, outBuffer, QUEUE_TICKS);
		if (result == pdTRUE )
		{
			cmd = outBuffer[0];
			//xSerialPutChar(pxWan, cmd, 5);
			switch (cmd)
			{
			case 'T':
			case 'E':
			case 'F':
			case 'G':
				if (!has_syncd)
				{
					synchronize_zigbit();
					has_syncd = true;
				}
				break;
			case CONFIG_RESP:
				xTaskNotifyGive(xWanTaskHandle)
				;
				break;
			case CHANGESET:
				led_alert_on();
				//update_changeset();
				break;
			}
		}
	}
}

void synchronize_zigbit()
{
	while (NWK_READY)
	{
		xSerialPutChar(pxWan, 'X', 5);
	}
}

void task_wan_dispatch_start(UBaseType_t uxPriority)
{
	if (!queue_created)
		xWanDispatchQueue = xQueueCreate( 10, BUFFER_SIZE );

	if (xWanDispatchQueue == 0)
	{
		led_alert_on();
	} else
	{
		queue_created = true;
		xTaskCreate(task_wan_dispatch, "wan_dispatch", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);

	}
}
