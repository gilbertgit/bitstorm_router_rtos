/*
 * ble_serial.c
 *
 *  Created on: Feb 19, 2015
 *      Author: titan
 */

#include <stdio.h>
#include <avr/interrupt.h>
#include "task_ble_serial.h"
#include "task_ble_dispatch.h"

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_SIZE		10
#define QUEUE_TICKS		10

static signed char inBuffer[BUFFER_MAX + 1];
static signed char outBuffer[BUFFER_SIZE];
volatile unsigned char len;

static int bufferIndex;
//static uint8_t command = 0;
static BaseType_t result;
static xComPortHandle pxBle;

bool ble_queue_created = false;

QueueHandle_t xBleQueue;

uint8_t inState = 0;
const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;
const static TickType_t xDelay_2s = 2000 / portTICK_PERIOD_MS;
void ble_usart_tx(xComPortHandle px, uint8_t data[], int size);

uint8_t discoverCmd[] = { 0x00, 0x01, 0x06, 0x02, 0x01 };
uint8_t endDiscoverCmd[] = { 0x00, 0x00, 0x06, 0x04 };
uint8_t discoverParams[] = { 0x00, 0x05, 0x06, 0x07, 0x40, 0x00, 0x32, 0x00, 0x00 };
uint8_t connectCmd[] = { 0x00, 0x0F, 0x06, 0x03, 0x12, 0xE1, 0x2D, 0x80, 0x07, 0x00, 0x00, 0x06, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x0A, 0x00 };
uint8_t disconnectCmd[] = { 0x00, 0x01, 0x03, 0x00, 0x00 };
uint8_t modeCmd[] = { 0x00, 0x02, 0x06, 0x01, 0x02, 0x02 };

static portTASK_FUNCTION(task_ble, params)
{
	pxBle = xSerialPortInitMinimal(1, 38400, 50);

	BaseType_t result;
	signed char inChar;

	xComPortHandle pxWan;
	pxWan = xSerialPortInitMinimal(0, 38400, 64); //WAN

	bufferIndex = 0;

	DDRD |= _BV(PD5);		// CTS - output
	DDRD &= ~_BV(PD4);		// RTS - input

	PORTD &= ~_BV(PD5);		// lower CTS to start the BLE stream

	config_ble();
	uint8_t msg_type = 0xFF;

	pxBle = xSerialPortInitMinimal(1, 38400, 50);
	for (;;)
	{
		result = xSerialGetChar(pxBle, &inChar, 5);
		if (result == pdTRUE )
		{
			uint8_t data = inChar;
			//xSerialPutChar(pxWan, data, 5);
			switch (inState)
			{
			case 0:
				//inState = (data == 0x80 || data == 0x00) ? 1 : 0;
				if (data == 0x80)
				{
					inBuffer[bufferIndex++] = data;
					inState = 1;
				}
				else if (data == 0x00)
				{
					inBuffer[bufferIndex++] = data;
					inState = 1;
				}
				else
					inState = 0;
				break;
			case 1:
				len = data + 2;
				inBuffer[bufferIndex++] = data;
				inState = 4;
				break;
			case 4:
				inBuffer[bufferIndex++] = data;
				len--;
				if (len == 0)
				{

					//led_alert_on();
					PORTD |= _BV(PD5);
//					if (inBuffer[9] == 0x00 && inBuffer[10] == 0x00)
//					{
					result = xQueueSendToBack( xDispatchQueue, inBuffer, 0);
					//}
//					xSerialPutChar(pxWan, 0xAA, 5);
//					xSerialPutChar(pxWan, 0xAA, 5);
//					xSerialPutChar(pxWan, 0xAA, 5);
//					for (int i = 0; i < bufferIndex;)
//					{
//						xSerialPutChar(pxWan, inBuffer[i++], 5);
//					}
//					xSerialPutChar(pxWan, 0xBB, 5);
//					xSerialPutChar(pxWan, 0xBB, 5);
//					xSerialPutChar(pxWan, 0xBB, 5);
					bufferIndex = 0;
					inState = 0;
					PORTD &= ~_BV(PD5);

				}
				break;
			}
		}
	}
}

static portTASK_FUNCTION(task_ble_tx, params)
{
	for (;;)
	{
		// send queued commands to the BLE
		result = xQueueReceive( xBleQueue, outBuffer, QUEUE_TICKS);
		if (result == pdTRUE )
		{
			uint8_t size = outBuffer[0];
			if (outBuffer[0] == 0x06 && outBuffer[1] == 0x02)
			{
				config_ble();
			} else
			{
				for (int i = 1; i < size;)
				{
					xSerialPutChar(pxBle, outBuffer[i++], 5);
				}
			}
		}
	}
}

void config_ble()
{
	ble_usart_tx(pxBle, endDiscoverCmd, 4);
	ble_usart_tx(pxBle, discoverParams, 9);
	ble_usart_tx(pxBle, discoverCmd, 5);
	ble_usart_tx(pxBle, modeCmd, 6);
}

void ble_usart_tx(xComPortHandle px, uint8_t data[], int size)
{
	for (int i = 0; i < size;)
	{
		xSerialPutChar(px, data[i++], 5);
	}
}

void task_ble_serial_start(UBaseType_t uxPriority)
{
	if (!ble_queue_created)
		xBleQueue = xQueueCreate( 10, BUFFER_SIZE );

	if (xBleQueue == 0)
	{
		led_alert_on();
	} else
	{
		ble_queue_created = true;
		xTaskCreate(task_ble, "ble", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);
		xTaskCreate(task_ble_tx, "ble_tx", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);

	}
}
