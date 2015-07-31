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

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10
#define WAN_CONFIG_RETRY_MAX	3
static signed char outBuffer[BUFFER_SIZE];
static uint8_t inBuffer[BUFFER_SIZE];
static uint8_t frame[80];
static uint8_t status_msg_frame[50];
static app_msg_t app_msg;
static cmd_send_header_t cmd_header;
static bool queue_created = false;

TaskHandle_t xWanTaskHandle;
TaskHandle_t xWanTaskHandle_rx;

QueueHandle_t xWANQueue;
uint16_t xWanMonitorCounter;

void sendtestdata();
UBaseType_t hwm;

enum states {
	CONFIGURE, WAIT_FOR_DATA, WAIT_FOR_NWK_BUSY, WAIT_FOR_NWK_READY, AWAITING_DATA, RECEIVED_DATA
};

static uint8_t state = CONFIGURE;
static uint8_t rx_state = AWAITING_DATA;

static int frame_index = 0;
static int frame_length = 0;
bool frame_ready = false;

static bool is_wan_configuring = false;
static int wan_config_retry = 0;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

xComPortHandle pxWan;

static uint16_t message_counter;
uint8_t cobs_buffer[60];
uint8_t decoded_msg[60];

changeset_t changeset;

static portTASK_FUNCTION(task_wan, params)
{
	uint8_t retries = 0;
	message_counter = 0;

	xWanMonitorCounter = 0;
	BaseType_t result;

	pxWan = xSerialPortInitMinimal(0, 38400, 5);
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

		if (router_config.magic == 2)
		{
			if (NWK_CONFIG) // PIN IS LOW SO CONFIGURE
			{
				// this is a blocking call... waits for WAN to configure
				configure_wan();
			} else if (NWK_READY)
			{
				result = xQueueReceive(xWANQueue, outBuffer, QUEUE_TICKS);

				if (result == pdTRUE )
				{
					PCMSK1 |= WAN_READY_ISR_MSK;
					retries = 0;
					for (;;)
					{
						if (outBuffer[0] == 0x08)
						{
							send_router_status_msg(pxWan, (router_status_msg_t*) outBuffer);
						} else
						{
							sendMessage(pxWan, (btle_msg_t *) outBuffer);
						}

						// wait for ready
						ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
						if (ulNotificationValue == 1)
						{
							// we're good, continue
							break;
						} else
						{
							// retry
							retries++;
							if (retries > 5)
							{
								kill_wan();
								vTaskDelay(xDelay);
								init_wan();
								break;
							}
						}
					}
					memset(outBuffer, 0, BUFFER_SIZE);
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
			// end of cobs message
			if (c == 0x00)
			{
				decode_cobs(cobs_buffer, sizeof(cobs_buffer), decoded_msg);
				index = 0;
				rx_state = RECEIVED_DATA;

			} else
			{
				cobs_buffer[index++] = c;
			}

			if (rx_state == RECEIVED_DATA)
			{
				result = xQueueSendToBack(xWanDispatchQueue, decoded_msg, 0);
				state = AWAITING_DATA;
			}
		}
	}
}

void sendtestdata()
{
	uint8_t test_frame[10];
	test_frame[0] = 0x09;
	test_frame[1] = 0x09;
	test_frame[2] = 0x89;
	test_frame[3] = 0x19;
	test_frame[4] = 0x45;
	test_frame[5] = 0xDD;
	test_frame[6] = 0x02;
	test_frame[7] = 0x09;
	test_frame[8] = 0xAA;
// message

// checksum
	uint8_t cs = 0;
	for (int i = 0; i < 9; cs ^= test_frame[i++])
		;
	test_frame[9] = cs;

	for (int i = 0; i < 10;)
	{
		xSerialPutChar(pxWan, test_frame[i++], 5);
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

//ISR(PCINT1_vect)
//{
//	//ERIC: Look here ... could we get into a loop inside the ISR? That would cause even the task switcher to lock.
//	//ERIC: Also, should we only enable this interrupt when we care about it?  ie, during configuration?
//	//ERIC: Should we make an effort to interrupt on
//	uint8_t value = PINB & (1 << PB0);
//	if (value == 0) // PIN IS LOW, SO WE SENT
//	{
//		//ERIC: Should check validity of xWanTaskHandle
//		//ERIC: Shoudl this be GIVE instead?
//		xTaskNotifyFromISR(xWanTaskHandle, WAN_BUSY, eSetBits, NULL );
//
//		//ERIC: Perhaps this is needed...
////		if (xHigherPriorityTaskWoken != pdFALSE) {
////				taskYIELD();
////			}
//	}
//}

void configure_wan()
{
	PCMSK1 |= WAN_CONFIG_ISR_MSK;

	uint32_t ulNotificationValue;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50);

	wan_config_retry = 0;
	for (;;)
	{
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
				kill_wan();
				vTaskDelay(xDelay);
				init_wan();
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

void send_router_status_msg(xComPortHandle hnd, router_status_msg_t * msg)
{
	msg->router_address = router_config.mac;
	msg->msg_sent_count = message_counter;
	msg->changeset_id = 0x0000;
	status_msg_frame[0] = sizeof(cmd_header) + sizeof(router_status_msg_t) + 1;

	cmd_header.command = CMD_SEND;
	cmd_header.pan_id = router_config.pan_id;
	cmd_header.short_id = router_config.mac & 0x0000FFFF;
	;
	cmd_header.message_length = sizeof(router_status_msg_t);

	int frame_index = 1;
// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		status_msg_frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(router_status_msg_t); i++)
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
}

void sendMessage(xComPortHandle hnd, btle_msg_t *msg)
{

	build_app_msg(msg, &app_msg);

	frame[0] = sizeof(cmd_header) + sizeof(app_msg) + 1;

	if (msg->type == MSG_TYPE_IN_PROX)
		app_msg.messageType = CMD_IN_PROX;
	else if (msg->type == MSG_TYPE_OUT_PROX)
		app_msg.messageType = CMD_OUT_PROX;

#ifdef ZB_ACK
	cmd_header.command = CMD_ACK_SEND;
#else
	cmd_header.command = CMD_SEND;
#endif
	cmd_header.pan_id = router_config.pan_id;
	cmd_header.short_id = 0x00;
	cmd_header.message_length = sizeof(app_msg);

	int frame_index = 1;
// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(app_msg_t); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&app_msg))[i];
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
	message_counter++;
//	hwm = uxTaskGetStackHighWaterMark(NULL);
//	sprintf((char*)frame, "A:%d\r\n", hwm);
//	for (int i=0; frame[i]; xSerialPutChar(pxWan, frame[i++], 5));
}

void build_app_msg(btle_msg_t *btle_msg, app_msg_t *msg)
{

	msg->messageType = 0x01;
	msg->nodeType = 0x01;
	msg->extAddr = btle_msg->mac;
	msg->shortAddr = router_config.mac & 0x0000FFFF;
	msg->routerAddr = router_config.mac;
//softVersion;
//channelMask;
	msg->panId = router_config.pan_id; // need to set pan in zigbit
	msg->workingChannel = router_config.channel;
	msg->parentShortAddr = 1;
	msg->lqi = 0;

	msg->rssi = btle_msg->rssi;
	msg->battery = btle_msg->batt;
	msg->temperature = btle_msg->temp;

// Calculate CS
//msg->cs = 0;
//for (int i = 0; i < sizeof(app_msg_t) - 1; msg->cs ^= ((uint8_t*) msg)[i++])
//	;

// ACTUALLY, don't calculate the CS, let the frame cs do the trick
	msg->cs = 0xCC;
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
