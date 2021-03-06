/*
 * wan_task.c
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#include <stdio.h>
#include <string.h>

#include "wan_task.h"
#include "wan.h"
#include "wan_msg.h"
#include "wan_config.h"
#include "../shared.h"
#include "../util/router_status_msg.h"
#include "task_wan_dispatch.h"
#include "util.h"

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10
#define WAN_CONFIG_RETRY_MAX	3
static signed char outBuffer[BUFFER_SIZE];
static uint8_t inBuffer[BUFFER_SIZE];
static uint8_t frame[80];
static uint8_t status_msg_frame[50];
static tag_msg_t tag_msg;
static cmd_send_header_t cmd_header;
static bool queue_created = false;

TaskHandle_t xWanTaskHandle;
TaskHandle_t xWanTaskHandle_rx;

QueueHandle_t xWANQueue;
uint16_t xWanMonitorCounter;

UBaseType_t hwm;

enum states {
	CONFIGURE, WAIT_FOR_DATA, WAIT_FOR_NWK_BUSY, WAIT_FOR_NWK_READY, AWAITING_DATA, RECEIVED_DATA
};

static int frame_index = 0;
static int frame_length = 0;
bool frame_ready = false;

static bool is_wan_configuring = false;
static int wan_config_retry = 0;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

xComPortHandle pxWan;

static uint16_t router_serial;
static uint32_t message_counter;
uint8_t cobs_buffer[60];
uint8_t decoded_msg[60];
uint8_t sync_count = 0;

changeset_t changeset;

static portTASK_FUNCTION(task_wan, params)
{
	init_wan();

	uint8_t retries = 0;
	router_serial = 0;
	message_counter = 0;

	xWanMonitorCounter = 0;
	BaseType_t result;

	pxWan = xSerialPortInitMinimal(0, 38400, 80);
	vTaskDelay(xDelay);		// Wait 500 to make sure all systems are up before we start sending

	read_config();

	uint32_t ulNotificationValue;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50);

	DDRA |= _BV(PA2);
	PORTA |= _BV(PA2);

	for (;;)
	{
		xWanMonitorCounter++;
		PORTA ^= _BV(PA2);

		TRACE(0x99);

		if (router_config.magic == 2)
		{
			if (NWK_CONFIG) // PIN IS LOW SO CONFIGURE
			{
				// this is a blocking call... waits for WAN to configure
				TRACE(0x02);
				configure_wan();
			} else if (NWK_READY)
			{
				TRACE(0x03);
				result = xQueueReceive(xWANQueue, outBuffer, QUEUE_TICKS);

				if (result == pdTRUE )
				{
					message_counter++;
					//ERIC: Should we read the status register first to clear it?
					//uint8_t sts_reg_read = PCMSK1;
					TRACE(0x04);
					PCMSK1 |= WAN_READY_ISR_MSK;

					//ERIC: Should this be wrapped in a critical section?
					//GE: the Enter_critical seems to make it error out more often, not sure why yet
					retries = 0;
					for (;;)
					{
						xWanMonitorCounter++;
						TRACE(0x05);

						if (outBuffer[0] == 0x08)
						{
							TRACE(0x06);
							send_router_status_msg(pxWan, (router_msg_t*) outBuffer);
						} else
						{
							TRACE(0x07);
							sendMessage(pxWan, (btle_msg_t *) outBuffer);
						}

						//ERIC: Maybe the PCMSK set/reset should happen closer, like here instead of outside the retry loop?

						// wait for ready
						TRACE(0x08);
						ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
						if (ulNotificationValue == 1)
						{
							// we're good, continue
							TRACE(0x09);
							break;
						} else
						{
							// retry
							retries++;
							if (retries > 5)
							{
								TRACE(0x77);
								if (sync_count <= 5)
									synchronize_zigbit();
								else
								{
									reboot_1284(WAN_MSG_RT);
								}
								break;
							}
						}
					}
					TRACE(0x0B);
					memset(outBuffer, 0, BUFFER_SIZE);
					TRACE(0x0C);
					PCMSK1 &= ~WAN_READY_ISR_MSK;
				}
			}
		}
	}
}

static portTASK_FUNCTION(task_wan_rx, params)
{
	signed char c;
	uint8_t index = 0;
	BaseType_t result;
	for (;;)
	{
		c = 0;
		while (xSerialGetChar(pxWan, &c, 0))
		{
//			if (index == 0 && (c == 'T' || c == 'G' || c == 'F' || c == 'G'))
//				break;

			// end of cobs message
			if (c == 0x00)
			{
				decode_cobs(cobs_buffer, sizeof(cobs_buffer), decoded_msg);
				result = xQueueSendToBack(xWanDispatchQueue, decoded_msg, 0);
				index = 0;
			} else
			{
				if (index > BUFFER_MAX)
					index = 0;
				cobs_buffer[index++] = c;
			}
		}
	}
}

void decode_cobs(const unsigned char *ptr, unsigned long length, unsigned char *dst)
{
	const unsigned char *end = ptr + length;
	while (ptr < end)
	{
		int i, code = *ptr++;
		for (i = 1; i < code; i++)
			*dst++ = *ptr++;
		if (code < 0xFF)
			*dst++ = 0;
	}
}

ISR(PCINT1_vect)
{
	uint8_t data = PINB;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	//ERIC: This is better. Maybe also check to see if the int mask is also set?  Just an additional guard.

	uint8_t ready_value = data & (1 << PB0);
	uint8_t config_value = data & (1 << PB1);
	if (config_value == 1 || ready_value == 1) // PIN IS HIGH, SO WAN IS CONFIGURED, OR WE READY TO SEND NEW MESSAGE
	{
		vTaskNotifyGiveFromISR(xWanTaskHandle, &xHigherPriorityTaskWoken);

		if (xHigherPriorityTaskWoken != pdFALSE )
		{
			taskYIELD();
		}
	}

}

void configure_wan()
{
	PCMSK1 |= WAN_CONFIG_ISR_MSK;

	uint32_t ulNotificationValue;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50);

	wan_config_retry = 0;
	for (;;)
	{
		//ERIC: Perhaps move the PCMSK set/unset closer to this line rather than outside retry loop.
		// now that we have the mac from the wan, send entire nwk config
		wan_config_network(pxWan);

		// wait for a response
		ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);

		if (ulNotificationValue == 1)
		{
			/* The transmission ended as expected. */
			frame_index = 0;
			is_wan_configuring = false;
			break;
		} else
		{
			/* The call to ulTaskNotifyTake() timed out. */
			wan_config_retry++;

			if (wan_config_retry > WAN_CONFIG_RETRY_MAX)
			{
				reboot_1284(WAN_CONFIG_RT);
				break;
			}
		}
	}
	PCMSK1 &= ~WAN_CONFIG_ISR_MSK;
}

bool isValidMessage(void)
{
	uint8_t cs = 0;
	uint8_t cs_in = 0;

	for (int i = 0; i < frame_length; cs ^= inBuffer[i++])
		;

	cs_in = inBuffer[frame_length];

	if (cs == cs_in)
	{
		return true;
	} else
	{
		return false;
	}
}

void send_router_status_msg(xComPortHandle hnd, router_msg_t * msg)
{

	msg->routerMac = router_config.mac;
	msg->routerShort = router_config.mac & 0x0000FFFF;
	msg->routerSerial = router_serial;
	msg->routerConfigSet = 0x00;
	msg->routerMsgCount = message_counter;
	msg->routerUptime = my_tick_count;
	msg->routerBattery = 0x00;
	msg->routerTemperature = 0x00;
	status_msg_frame[0] = sizeof(cmd_header) + sizeof(router_msg_t) + 1;

	cmd_header.command = CMD_SEND;
	cmd_header.pan_id = router_config.pan_id;
	cmd_header.short_id = router_config.mac & 0x0000FFFF;
	cmd_header.message_length = sizeof(router_msg_t);

	int frame_index = 1;
// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		status_msg_frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(router_msg_t); i++)
	{
		status_msg_frame[frame_index++] = ((uint8_t *) (msg))[i];
	}
// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= status_msg_frame[i++])
		;
	status_msg_frame[frame_index++] = cs;

	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(hnd, status_msg_frame[i++], 5);
	}
	router_serial++;
}

void sendMessage(xComPortHandle hnd, btle_msg_t *msg)
{

	build_app_msg(msg, &tag_msg);

	frame[0] = sizeof(cmd_header) + sizeof(tag_msg) + 1;

	if (msg->type == MSG_TYPE_IN_PROX)
		tag_msg.messageType = CMD_IN_PROX;
	else if (msg->type == MSG_TYPE_OUT_PROX)
		tag_msg.messageType = CMD_OUT_PROX;

#ifdef ZB_ACK
	cmd_header.command = CMD_ACK_SEND;
#else
	cmd_header.command = CMD_SEND;
#endif
	cmd_header.pan_id = router_config.pan_id;
	cmd_header.short_id = 0x00;
	cmd_header.message_length = sizeof(tag_msg);

	int frame_index = 1;
// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(tag_msg_t); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&tag_msg))[i];
	}
// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= frame[i++])
		;
	frame[frame_index++] = cs;

// send bytes
	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(hnd, frame[i++], 5);
	}

// keep track of how many messages we have sent out
	router_serial++;
//	hwm = uxTaskGetStackHighWaterMark(NULL);
//	sprintf((char*)frame, "A:%d\r\n", hwm);
//	for (int i=0; frame[i]; xSerialPutChar(pxWan, frame[i++], 5));
}

void build_app_msg(btle_msg_t *btle_msg, tag_msg_t *msg)
{

	msg->messageType = 0x01;
	msg->routerMac = router_config.mac;
	msg->routerShort = router_config.mac & 0x0000FFFF;
	msg->tagMac = btle_msg->mac;
	msg->tagConfigSet = btle_msg->cs_id;
	msg->tagSerial = btle_msg->tagSerial;
	msg->tagStatus = btle_msg->tagStatus;
	msg->tagLqi = 0x00;
	msg->tagRssi = btle_msg->rssi;
	msg->tagBattery = 0x00;
	msg->tagTemperature = 0x00;
}

void wan_reset_frame(void)
{
	frame_ready = false;
	frame_index = 0;
}

void task_wan_start(UBaseType_t uxPriority)
{
	if (!queue_created)
		xWANQueue = xQueueCreate(10, BUFFER_SIZE);

	if (xWANQueue == 0)
	{
		//led_alert_on();
	} else
	{
		queue_created = true;
		xTaskCreate(task_wan, "wan", 300, NULL, uxPriority, &xWanTaskHandle);
		xTaskCreate(task_wan_rx, "wan_rx", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xWanTaskHandle_rx);
	}
}
