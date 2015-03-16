/*
 * wan_task.c
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#include <stdio.h>

#include "../ble/task_ble_dispatch.h"
#include "wan_task.h"
#include "wan_msg.h"
#include "wan_config.h"
#include "../shared.h"

#define NWK_READY (PINB & (1 << PB0))
#define NWK_BUSY  (~(PINB & (1 << PB0)))

#define NWK_CONFIG (PINB & (1 << PB1))

#define WAN_BUSY 0x01

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10
static signed char outBuffer[BUFFER_SIZE];
static uint8_t inBuffer[BUFFER_SIZE];
static uint8_t frame[80];
static app_msg_t app_msg;
static cmd_send_header_t cmd_header;
static TaskHandle_t xHandlingTask;
static unsigned long ulNotifiedValue;

QueueHandle_t xWANQueue;

enum states {
	CONFIGURE, WAIT_FOR_DATA, WAIT_FOR_NWK_BUSY, WAIT_FOR_NWK_READY
};
static uint8_t state = CONFIGURE;
static int frame_index = 0;
static int frame_length = 0;
bool frame_ready = false;

const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;
const static TickType_t xDelaySend = 50 / portTICK_PERIOD_MS;

static xComPortHandle pxWan;
//static UBaseType_t hwm = 0;
bool ready_to_send = false;


static portTASK_FUNCTION(task_wan, params)
{

	BaseType_t result;

	pxWan = xSerialPortInitMinimal(0, 38400, 50);
	vTaskDelay(xDelay);		// Wait 500

	for (;;)
	{

		if (!(PINB & (1 << PB1))) // PIN IS LOW SO CONFIGURE
		{
			configure_wan();
		} else
		{
				if (NWK_READY)
				{
					//vTaskDelay(xDelaySend); // hack to slow down the messages sent
					result = xQueueReceive( xWANQueue, outBuffer, QUEUE_TICKS);
					if (result == pdTRUE )
					{
						sendMessage(pxWan, (btle_msg_t *) outBuffer);

						 result = xTaskNotifyWait( pdFALSE,          /* Don't clear bits on entry. */
								 	 	 	 	 	 	   0xffffffff,        /* Clear all bits on exit. */
						                                   &ulNotifiedValue, /* Stores the notified value. */
						                                   xDelaySend );

						 // TODO: handle timeout errors
					}
				}
			//}
		}

	}
}

ISR(PCINT1_vect)
{

	uint8_t value = PINB & (1 << PB0);
	if (value == 0) // PIN IS HIGH, SO WE CAN SEND
	{
		xTaskNotifyFromISR( xHandlingTask, WAN_BUSY, eSetBits, NULL );
	}
}

void configure_wan()
{
	wan_get_device_address();
	waitForResp();
	config_mac_resp((mac_resp_t *) &inBuffer[1]);
	frame_index = 0;

	wan_config_network();
	waitForResp();
	config_ntw_resp((config_ntw_resp_t *) &inBuffer[1]);
	frame_index = 0;

}

void waitForResp()
{
	BaseType_t result;
	signed char inChar;
	bool waiting = true;
	while (waiting)
	{
		result = xSerialGetChar(pxWan, &inChar, 0xFFFF);

		if (result == pdTRUE )
		{
			inBuffer[frame_index] = inChar;
			if (frame_index == 0)
			{
				frame_length = inBuffer[frame_index];
			} else if (frame_index >= frame_length)
			{
				frame_ready = true;
				waiting = false;
			}

			frame_index++;
		}
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
	cmd_header.pan_id = 0x1973;
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

//	hwm = uxTaskGetStackHighWaterMark(NULL);
//	sprintf((char*)frame, "B:%d ", hwm);
//	for (int i=0; frame[i]; xSerialPutChar(pxWan, frame[i++], 5));

	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(hnd, frame[i++], 5);
		//xSerialPutChar(pxWan, 'X', 5); i++;
	}

//	hwm = uxTaskGetStackHighWaterMark(NULL);
//	sprintf((char*)frame, "A:%d\r\n", hwm);
//	for (int i=0; frame[i]; xSerialPutChar(pxWan, frame[i++], 5));
}

void build_app_msg(btle_msg_t *btle_msg, app_msg_t *msg)
{

	msg->messageType = 0x01;
	msg->nodeType = 0x01;
	msg->extAddr = btle_msg->mac;
	msg->shortAddr = shared.mac & 0x0000FFFF;
	msg->routerAddr = shared.mac;
	//softVersion;
	//channelMask;
	msg->panId = 0x1973; // need to set pan in zigbit
	msg->workingChannel = 0x16;
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

void wan_state_configure(void)
{
	if (wan_config())
	{
		state = WAIT_FOR_DATA;
	} else if (frame_ready)
	{
		if (wan_config_received(inBuffer))
		{
			state = WAIT_FOR_DATA;
		}
		wan_reset_frame();
	}
}

void task_wan_start(UBaseType_t uxPriority)
{
	xWANQueue = xQueueCreate( 10, BUFFER_SIZE );
	if (xWANQueue == 0)
	{
		//led_alert_on();
	} else
	{
		xTaskCreate(task_wan, "wan", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xHandlingTask);
	}
}
