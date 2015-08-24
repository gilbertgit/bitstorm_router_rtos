/*
 * task_wan_dispatch.c
 *
 *  Created on: Jul 6, 2015
 *      Author: titan
 */
#include <stdbool.h>
#include <avr/eeprom.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"
#include "task_wan_dispatch.h"
#include "wan_task.h"
#include "../ble/task_ble_serial.h"
#include "../shared.h"
#include "util.h"

#define NWK_READY (PINB & (1 << PB0))
#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10

enum comands {
	CONFIG_RESP = 0x03, CHANGESET = 0X09, WAN_STATUS = 0xEF
};
enum wan_statuses {
	MSG_TIMEOUT = 0x0A, CONF_RETRIES = 0x0B, MSG_ERROR = 0x0C, INVALID_MSG = 0x0D, SYNC_CONF = 0x0E
};

static signed char outBuffer[BUFFER_SIZE];
static BaseType_t result;
bool queue_created = false;

QueueHandle_t xWanDispatchQueue;
TaskHandle_t xWanDispatchHandle;
changeset_t changeset;
uint16_t xWanDispatchMonitorCounter;

// Create variable in EEPROM with initial values
changeset_t EEMEM changeset_temp = { 0x00 };

static portTASK_FUNCTION(task_wan_dispatch, params)
{
	uint8_t cmd;
	uint8_t wan_status;
	read_changeset();

	for (;;)
	{
		xWanDispatchMonitorCounter++;
		result = xQueueReceive( xWanDispatchQueue, outBuffer, QUEUE_TICKS);
		if (result == pdTRUE )
		{
			cmd = outBuffer[0];
			switch (cmd)
			{
			case CHANGESET:
				update_changeset();
				xQueueSendToBack(xBleQueue, outBuffer, 0);
				break;
			case WAN_STATUS:
				// successful sync will clear the counter
				wan_status = outBuffer[1];
				switch (wan_status)
				{
				case SYNC_CONF:
					sync_count = 0;
					break;
				case CONF_RETRIES:
					// not cool. LWM had an issue and couldn't get a
					// message to the coordinator after 3 retries. REBOOT!
					reboot_1284(CONF_RETRIES);
					break;
				}
				break;
			}
		}
	}
}

void update_changeset()
{
	changeset.id = outBuffer[1];
	eeprom_update_block(&changeset, &changeset_temp, sizeof(changeset_t));
}
void read_changeset()
{
	eeprom_read_block(&changeset, &changeset_temp, sizeof(changeset_t));
}

void synchronize_zigbit()
{
	while (NWK_READY)
	{
		xSerialPutChar(pxWan, 'X', 5); // 'X' is 58 in hex
	}
	sync_count++;
}

void task_wan_dispatch_start(UBaseType_t uxPriority)
{
	if (!queue_created)
		xWanDispatchQueue = xQueueCreate( 10, BUFFER_SIZE );

	if (xWanDispatchQueue == 0)
	{
//		led_alert_on();
	} else
	{
		queue_created = true;
		xTaskCreate(task_wan_dispatch, "wan_dispatch", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);

	}
}
