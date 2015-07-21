/*
 * wan_task.c
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#include <stdio.h>

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
static signed char outBuffer[BUFFER_SIZE];
static uint8_t inBuffer[BUFFER_SIZE];
static uint8_t frame[80];
static uint8_t status_msg_frame[50];
static app_msg_t app_msg;
static cmd_send_header_t cmd_header;
static unsigned long ulNotifiedValue;
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

static bool wan_config_good = false;
static int wan_config_retry = 0;

const static TickType_t xDelayTest = 50 / portTICK_PERIOD_MS;
const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;
const static TickType_t xDelaySend = 500 / portTICK_PERIOD_MS;

static xComPortHandle pxWan;

static uint16_t message_counter;
uint8_t cobs_buffer[60];
uint8_t decoded_msg[60];

changeset_t changeset;

// Create variable in EEPROM with initial values
//changeset_t EEMEM changeset_temp = { 12345, 1234, 1 };

static portTASK_FUNCTION(task_wan, params) {

	message_counter = 0;

	xWanMonitorCounter = 0;
	BaseType_t result;

	pxWan = xSerialPortInitMinimal(0, 38400, 50);
	vTaskDelay(xDelay);		// Wait 500 to make sure all systems are up before we start sending

	// bounce power to wan
	// read config
	// if config is good (magic==2) then push 0x09/0x09 onto queue

	for (;;) {
		xWanMonitorCounter++;

		result = xQueueReceive(xWANQueue, outBuffer, QUEUE_TICKS);

		if (result == pdTRUE) {
			if (NWK_CONFIG) // PIN IS LOW SO CONFIGURE
			{
				if (outBuffer[0] == 0x09 && outBuffer[1] == 0x09) {
					configure_wan();
				}
			} else if (NWK_READY) {
				if (outBuffer[0] == 8) {
					send_router_status_msg(pxWan, (router_status_msg_t*) outBuffer);
				} else if (outBuffer[0] == 0x09 && outBuffer[1] == 0x09) {
					configure_wan();
				} else {
					sendMessage(pxWan, (btle_msg_t *) outBuffer);
				}

				result = xTaskNotifyWait(pdFALSE, /* Don't clear bits on entry. */
				0xffffffff, /* Clear all bits on exit. */
				&ulNotifiedValue, /* Stores the notified value. */
				xDelaySend);

				if (result == pdFALSE)
					led_alert_on();
				else
					led_alert_off();

				// TODO: handle timeout errors
			}
		}
	}
}

static portTASK_FUNCTION(task_wan_rx, params) {
	signed char c;
	uint8_t index = 0;
	BaseType_t result;
	for (;;) {
		c = 0;
		while (xSerialGetChar(pxWan, &c, 0)) {
			// end of cobs message
			if (c == 0x00) {
				decode_cobs(cobs_buffer, sizeof(cobs_buffer), decoded_msg);
				index = 0;
				rx_state = RECEIVED_DATA;

			} else {
				cobs_buffer[index++] = c;
			}

			if (rx_state == RECEIVED_DATA) {
				result = xQueueSendToBack(xWanDispatchQueue, decoded_msg, 0);
				state = AWAITING_DATA;
			}
		}
	}
}

void sendtestdata() {
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

	for (int i = 0; i < 10;) {
		xSerialPutChar(pxWan, test_frame[i++], 5);
	}
}

void decode_cobs(const unsigned char *ptr, unsigned long length, unsigned char *dst) {
	const unsigned char *end = ptr + length;
	while (ptr < end) {
		int i, code = *ptr++;
		for (i = 1; i < code; i++)
			*dst++ = *ptr++;
		if (code < 0xFF)
			*dst++ = 0;
	}
}

void synchronize() {
	while (NWK_READY) {
		xSerialPutChar(pxWan, 'X', 5);
	}
}

ISR(PCINT1_vect) {
	uint8_t value = PINB & (1 << PB0);
	if (value == 0) // PIN IS LOW, SO WE SENT
			{
		xTaskNotifyFromISR(xWanTaskHandle, WAN_BUSY, eSetBits, NULL);
	}
}

void configure_wan() {
	wan_config_retry++;
	uint32_t ulNotificationValue;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200);

// now that we have the mac from the wan, send entire nwk config
	wan_config_network(pxWan);

// wait for a response
	ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);

	if (ulNotificationValue == 1) {
		/* The transmission ended as expected. */
		frame_index = 0;
		wan_config_done();

		frame_index = 0;

		vTaskDelay(xDelayTest);

		led_alert_off();
	} else {
		/* The call to ulTaskNotifyTake() timed out. */
		configure_wan();
		if (wan_config_retry == 3) {
			synchronize();
			vTaskDelay(xDelay);
			configure_wan();
		}
	}
}

bool isValidMessage(void) {
	uint8_t cs = 0;
	uint8_t cs_in = 0;

	for (int i = 0; i < frame_length; cs ^= inBuffer[i++])
		;

	cs_in = inBuffer[frame_length];

	if (cs == cs_in) {
		return true;
	} else {
		return false;
	}
}
void waitForNwkConfigResp2() {
	uint8_t nwk_config_wait_counter = 0;

	while (NWK_CONFIG) {

		if (wan_config_good == true)
			break;

		nwk_config_wait_counter++;
		// don't wait forever, restart process if we wait too long
		if (nwk_config_wait_counter >= 2000) {

			synchronize();
		}
	}
}
void waitForNwkConfigResp() {
	BaseType_t result;
	signed char inChar;
	uint8_t nwk_config_wait_counter = 0;
	while (NWK_CONFIG) {
		nwk_config_wait_counter++;
		result = xSerialGetChar(pxWan, &inChar, 5);

		if (result == pdTRUE) {
			inBuffer[frame_index] = inChar;
			if (frame_index == 0) {
				frame_length = inBuffer[frame_index];
			} else if (frame_index >= frame_length) {

				if (isValidMessage()) {
					led_alert_on();
					// we got a good message so continue with the config
					break;

				} else {
					// if the message was bad we need to retry getting the address
					wan_config_good = false;
				}
			}

			frame_index++;
		}
		// don't wait forever, restart process if we wait too long
		if (nwk_config_wait_counter >= 2000) {
			synchronize();
		}
	}
}

void send_router_status_msg(xComPortHandle hnd, router_status_msg_t * msg) {
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
	for (int i = 0; i < sizeof(cmd_header); i++) {
		status_msg_frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(router_status_msg_t); i++) {
		status_msg_frame[frame_index++] = ((uint8_t *) (msg))[i];
	}
// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= status_msg_frame[i++])
		;
	status_msg_frame[frame_index++] = cs;

	for (int i = 0; i < frame_index;) {
		xSerialPutChar(hnd, status_msg_frame[i++], 5);
	}
}

void sendMessage(xComPortHandle hnd, btle_msg_t *msg) {

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
	for (int i = 0; i < sizeof(cmd_header); i++) {
		frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
// message
	for (int i = 0; i < sizeof(app_msg_t); i++) {
		frame[frame_index++] = ((uint8_t *) (&app_msg))[i];
	}
// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= frame[i++])
		;
	frame[frame_index++] = cs;

// send bytes
	for (int i = 0; i < frame_index;) {
		xSerialPutChar(hnd, frame[i++], 5);
	}

// keep track of how many messages we have sent out
	message_counter++;
//	hwm = uxTaskGetStackHighWaterMark(NULL);
//	sprintf((char*)frame, "A:%d\r\n", hwm);
//	for (int i=0; frame[i]; xSerialPutChar(pxWan, frame[i++], 5));
}

void build_app_msg(btle_msg_t *btle_msg, app_msg_t *msg) {

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

void wan_reset_frame(void) {
	frame_ready = false;
	frame_index = 0;
}

void wan_state_configure(void) {
//	if (wan_config())
//	{
//		state = WAIT_FOR_DATA;
//	} else if (frame_ready)
//	{
//		if (wan_config_received(inBuffer))
//		{
//			state = WAIT_FOR_DATA;
//		}
//		wan_reset_frame();
//	}
}

void task_wan_start(UBaseType_t uxPriority) {
	if (!queue_created)
		xWANQueue = xQueueCreate(10, BUFFER_SIZE);

	if (xWANQueue == 0) {
		//led_alert_on();
	} else {
		queue_created = true;
		xTaskCreate(task_wan, "wan", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xWanTaskHandle);
		xTaskCreate(task_wan_rx, "wan_rx", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xWanTaskHandle_rx);

	}
}
