/*
 * task_ble_dispatch.c
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */
#include <stdbool.h>
#include <string.h>

#include "task_ble_serial.h"
#include "task_ble_dispatch.h"
#include "ble_msg.h"
#include "../wan/wan_task.h"
#include "../ramdisk/ramdisk.h"
#include "../util/clock.h"

char HEX_DIGITS[] = "0123456789abcdef";

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10

static signed char outBuffer[BUFFER_SIZE];

QueueHandle_t xDispatchQueue;

static portTASK_FUNCTION(task_dispatch, params)
{

	BaseType_t result;
	btle_msg_t msg;

	for (;;)
	{
		result = xQueueReceive( xDispatchQueue, outBuffer, QUEUE_TICKS);
		if (result == pdTRUE )
		{
			// TODO: Make decisions on the messages from the ble
			btle_handle_le_packet((char *) outBuffer, &msg);

#ifdef BYPASS_MODE
			msg.type = MSG_TYPE_NORM;
			result = xQueueSendToBack( xWANQueue, &msg, 0);
#else
				if (msg.mac != 0)
				{
					btle_msg_t *temp = ramdisk_find(msg.mac);

					// no package in ramdisk
					if (temp == NULL )
					{
						// new packet
						msg.last_sent = clock_time();
						msg.count = 1;
						ramdisk_write(msg);
						msg.type = MSG_TYPE_IN_PROX;
						result = xQueueSendToBack( xWANQueue, &msg, 0);

						// send in proximity
					} else
					{
						// package in RAM
						// update packet
						temp->rssi = msg.rssi;
						temp->batt = msg.batt;
						temp->temp = msg.temp;
						temp->count = temp->count + 1;

						// is the packet stale?
						if ((clock_time() - temp->last_sent) > 5000)
						{
							// send standard packet

							temp->type = MSG_TYPE_NORM;
							result = xQueueSendToBack( xWANQueue, temp, 0);
							temp->last_sent = clock_time();
						}

					}
				}
#endif
			if (result != pdTRUE )
			{
				led_alert_on();
			}
		}

	}
}

bool btle_handle_le_packet(char * buffer, btle_msg_t * btle_msg)
{

	memset(btle_msg, 0, sizeof(btle_msg_t));

	//           1111111111222222222
	// 01234567890123456789012345678
	// |||||||||||||||||||||||||||||
	// *00078072CCB3 C3 5994 63BC 24

	uint8_t * num;
	uint8_t msb, lsb, ck, ckx;
	uint8_t rssi;
	uint16_t batt, temp;
	uint64_t mac;
	int i;

	// Validate checksum in bytes 27-28
	// Just an XOR of bytes 0-26
	msb = btle_parse_nybble(buffer[27]);
	lsb = btle_parse_nybble(buffer[28]);
	ck = (msb << 4) | lsb;
	ckx = 0;
	for (i = 0; i <= 26; i++)
		ckx ^= buffer[i];
	if (ck != ckx)
	{
		return false;
	}

	// MAC address - incoming 48bits
	//
	num = (uint8_t *) &mac;
	num[7] = 0;
	num[6] = 0;
	msb = btle_parse_nybble(buffer[1]);
	lsb = btle_parse_nybble(buffer[2]);
	num[5] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[3]);
	lsb = btle_parse_nybble(buffer[4]);
	num[4] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[5]);
	lsb = btle_parse_nybble(buffer[6]);
	num[3] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[7]);
	lsb = btle_parse_nybble(buffer[8]);
	num[2] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[9]);
	lsb = btle_parse_nybble(buffer[10]);
	num[1] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[11]);
	lsb = btle_parse_nybble(buffer[12]);
	num[0] = (msb << 4) | lsb;

	// RSSI
	//
	msb = btle_parse_nybble(buffer[14]);
	lsb = btle_parse_nybble(buffer[15]);
	rssi = (msb << 4) | lsb;

	// Temperature
	//
	num = (uint8_t *) &temp;
	msb = btle_parse_nybble(buffer[17]);
	lsb = btle_parse_nybble(buffer[18]);
	num[0] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[19]);
	lsb = btle_parse_nybble(buffer[20]);
	num[1] = (msb << 4) | lsb;

	// Battery
	//
	num = (uint8_t *) &batt;
	msb = btle_parse_nybble(buffer[22]);
	lsb = btle_parse_nybble(buffer[23]);
	num[0] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[24]);
	lsb = btle_parse_nybble(buffer[25]);
	num[1] = (msb << 4) | lsb;

	btle_msg->rssi = rssi;
	btle_msg->mac = mac;
	btle_msg->batt = batt;
	btle_msg->temp = temp;

	return true;

}

uint8_t btle_parse_nybble(char c)
{
	if (c >= 'A' && c <= 'F')
		c = c | 0x20;
	for (uint8_t i = 0; i < 16; i++)
	{
		if (HEX_DIGITS[i] == c)
			return i;
	}
	return 0x80;
}

void task_ble_dispatch_start(UBaseType_t uxPriority)
{
	xDispatchQueue = xQueueCreate( 10, BUFFER_SIZE );
	if (xDispatchQueue == 0)
	{
		led_alert_on();
	} else
	{
		xTaskCreate(task_dispatch, "dispatch", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);
	}
}
