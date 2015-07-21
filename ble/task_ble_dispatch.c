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
#include <avr/eeprom.h>
#include "../wan/wan_task.h"
#include "../wan/wan_config.h"
#include "../ramdisk/ramdisk.h"
#include "../util/clock.h"
#include "../shared.h"

char HEX_DIGITS[] = "0123456789abcdef";

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_TICKS		10

static signed char outBuffer[BUFFER_SIZE];
static int bufferIndex;
static uint8_t command = 0;

static BaseType_t result;
static btle_msg_t msg;

QueueHandle_t xDispatchQueue;
router_config_t router_config;
//changeset_t cs_v;

// Create variable in EEPROM with initial values
router_config_t EEMEM router_config_temp = { 12345678, 1234, 1 };

#define RSSI_INDEX 			0
#define PACKET_TYPE_INDEX 	1
#define ADDRESS_INDEX 		2
#define ADDRESS_TYPE_INDEX 	8
#define BOND_INDEX 			9
#define DATA_LENGTH_INDEX 	10
#define DATA_INDEX 			11

enum classes {
	SYSTEM_CLASS = 0x00, ATTR_DB_CLASS = 0x02, CONNECTION = 0X03, GAP_CLASS = 0x06
};
uint64_t router_addr_temp;
const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;

static portTASK_FUNCTION(task_dispatch, params)
{
	//xComPortHandle pxOut;
	//pxOut = xSerialPortInitMinimal(0, 38400, 10);
	read_config();
	read_changeset();
	for (;;)
	{
		result = xQueueReceive( xDispatchQueue, outBuffer, QUEUE_TICKS);
		if (result == pdTRUE )
		{
			switch (outBuffer[2])
			{
			case ATTR_DB_CLASS:
				attr_db_class();
				break;
			case GAP_CLASS:
				gap_class();
				break;
			case SYSTEM_CLASS:
				system_class();
				break;
			case CONNECTION:
				connection_class();
				break;
			}

			// TODO: check if the tag that sent the message has the correct changeset
			// if not correct, put in ble queue that changeset needs to be updated for this tag
			if (msg.cs_id == changeset.id)
				led_alert_on();

#ifdef BYPASS_MODE
//			msg.type = MSG_TYPE_NORM;
//			result = xQueueSendToBack( xWANQueue, &msg, 0);
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

					//					if((clock_time() - temp->last_sent) > 10000)
					//						led_alert_on();

					// is the packet stale?
					//if ((clock_time() - temp->last_sent) > 5000)
					{
						// send standard packet
						temp->type = MSG_TYPE_NORM;
						temp->last_sent = clock_time();
						result = xQueueSendToBack( xWANQueue, temp, 0);

					}

				}
			}
#endif

//			bufferIndex = 0;
//			if (result != pdTRUE )
//			{
//				//led_alert_on();
//			}
//			command = 0;
//
//			if (router_config.magic != 2)
//			{
//				led_alert_toggle();
//				continue;
//			}
		}

	}
}

void system_class(xComPortHandle hnd)
{
	switch (outBuffer[3])
	{
	case 0x02: // address response
		if (router_config.magic == 1)
		{
			router_config.mac = *(uint64_t*) &outBuffer[4];
			router_config.magic = 2;
			write_config();
		}
		break;
	}
}

void attr_db_class()
{
	uint8_t getAddrCmd[] = { 0x05, 0x00, 0x00, 0x00, 0x02 };

	switch (outBuffer[3])
	{
	case 0x00: // GATT value change
		handle_router_config_packet2((char*) outBuffer, &router_config);

		// get the ble address to finish the config
		xQueueSendToBack(xBleQueue, getAddrCmd, 0);

		break;
	}
}

void gap_class()
{
	// what is the method
	switch (outBuffer[3])
	{
	case 0x00: // discover event
		// TODO: filter data that is not from BS tags
		btle_handle_le_packet2((char *) &outBuffer[4], &msg);
		if (router_config.magic == 2)
		{
			msg.type = MSG_TYPE_NORM;
			result = xQueueSendToBack( xWANQueue, &msg, 0);
		}
//		for (int i = 0; i < sizeof(btle_msg_t);)
//		{
//			//xSerialPutChar(hnd, ((char *) &outBuffer[2])[i], 5);
//			xSerialPutChar(hnd,((char *) (&msg))[i++], 5);
//		}
		break;
	}
}

void connection_class()
{
	uint8_t configCmd[] = { 0x06, 0x02 };
	uint8_t configWanCmd[] = { 0x09, 0x09 };
	switch (outBuffer[3])
	{
	case 0x04: // disconnect event
		led_alert_toggle();

		if (router_config.magic == 2)
		{
			read_config();
			vTaskDelay(xDelay);
			xQueueSendToBack(xWANQueue, configWanCmd, 0);
		}
		xQueueSendToBack(xBleQueue, configCmd, 0);
		break;
	}
}

bool btle_handle_le_packet2(char * msg, btle_msg_t * btle_msg)
{
	memset(btle_msg, 0, sizeof(btle_msg_t));
	PACKAGE* pkg = malloc(sizeof(PACKAGE));

	//int i;
	pkg->rssi = msg[RSSI_INDEX];
	pkg->packetType = msg[PACKET_TYPE_INDEX];

	pkg->address = (uint64_t) msg[6] << 32 | (uint64_t) msg[5] << 24 | (uint64_t) msg[4] << 16 | (uint64_t) msg[3] << 8 | (uint64_t) msg[2];
	router_addr_temp = pkg->address;
	//pkg->address = *((uint64_t*) &msg[0x02]);
//	for (i = ADDRESS_INDEX; i < ADDRESS_TYPE_INDEX; i++)
//	{
//		pkg->address[i - ADDRESS_INDEX] = msg[i];
//	}
//	pkg->addressType = msg[ADDRESS_TYPE_INDEX];
//	pkg->bond = msg[BOND_INDEX];
//	pkg->dataLen = msg[DATA_LENGTH_INDEX];
//	for (i = 0; i < pkg->dataLen; i++)
//	{
//		pkg->data[i] = msg[i + DATA_INDEX];
//	}

	btle_msg->rssi = pkg->rssi;
	btle_msg->mac = pkg->address;
	btle_msg->batt = 0;
	btle_msg->temp = 0;
	btle_msg->cs_id = 0xAB;

	return true;
}

bool handle_router_config_packet2(char * buffer, router_config_t * conf)
{
	memset(conf, 0, sizeof(router_config_t));
	conf->pan_id = *((uint16_t*) &buffer[11]);
	conf->channel = buffer[13];
	conf->magic = 1;
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

void read_config()
{

	eeprom_read_block(&router_config, &router_config_temp, sizeof(router_config_t));
}

void write_config()
{
//	router_config.magic = 0;
//	router_config.mac = 0;
//	router_config.pan_id = 0;
//	router_config.channel = 0;
	eeprom_update_block(&router_config, &router_config_temp, sizeof(router_config_t));
}

void task_ble_dispatch_start(UBaseType_t uxPriority)
{
	xDispatchQueue = xQueueCreate( 10, BUFFER_SIZE );
	if (xDispatchQueue == 0)
	{
		//led_alert_on();
	} else
	{
		xTaskCreate(task_dispatch, "dispatch", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);
	}
}
