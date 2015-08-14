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
#include "ble.h"

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_SIZE		10
#define QUEUE_TICKS		10
#define READY_TO_RCV		PORTD &= ~BLE_CTS_bv;
#define NOT_READY_TO_RCV	PORTD |= BLE_CTS_bv

static signed char inBuffer[BUFFER_MAX + 1];
static signed char outBuffer[BUFFER_SIZE];
volatile unsigned char len;

static int bufferIndex;
//static uint8_t command = 0;
static BaseType_t result;
static xComPortHandle pxBle;

bool ble_queue_created = false;

QueueHandle_t xBleQueue;
uint16_t xBleMonitorCounter;

uint8_t inState = 0;
const static TickType_t xDelay = 500 / portTICK_PERIOD_MS;
void ble_usart_tx(xComPortHandle px, uint8_t data[], int size);

uint8_t endDiscoverCmd[] = { 0x00, 0x00, 0x06, 0x04 };
uint8_t connectCmd[] = { 0x00, 0x0F, 0x06, 0x03, 0x12, 0xE1, 0x2D, 0x80, 0x07, 0x00, 0x00, 0x06, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x0A, 0x00 };
uint8_t disconnectCmd[] = { 0x00, 0x01, 0x03, 0x00, 0x00 };
uint8_t helloCmd[] = { 0x00, 0x00, 0x00, 0x01 };

static portTASK_FUNCTION(task_ble, params)
{
	init_ble();

	xBleMonitorCounter = 0;
	BaseType_t result;
	signed char inChar;

	bufferIndex = 0;

	pxBle = xSerialPortInitMinimal(1, 38400, 80);

	// I have to send this in order for the BG to let me know that it is ready to receive messages.
	hello_ble();

	for (;;)
	{
		xBleMonitorCounter++;

		result = xSerialGetChar(pxBle, &inChar, 5);

		if (result == pdTRUE )
		{
			uint8_t data = inChar;

			switch (inState)
			{
			case 0:
				//inState = (data == 0x80 || data == 0x00) ? 1 : 0;
				if (data == 0x80 || data == 0x00)
				{
					inBuffer[bufferIndex++] = data;
					inState = 1;
				} else
				{
					reset_transmit();
					bufferIndex = 0;
					inState = 0;
				}
				break;
			case 1:
				len = data + 2;
				if (len > BUFFER_MAX)
				{
					reset_transmit();
					bufferIndex = 0;
					inState = 0;
				} else
				{
					inBuffer[bufferIndex++] = data;
					inState = 4;
				}
				break;
			case 4:
				inBuffer[bufferIndex++] = data;
				len--;
				if (len == 0)
				{
					NOT_READY_TO_RCV;

					result = xQueueSendToBack( xDispatchQueue, inBuffer, 0);

					bufferIndex = 0;
					inState = 0;
					//ERIC: Perhaps empty the inbound serial port queue here?  Probably not a problem.
//					for (;;)
//					{
//						result = xSerialGetChar(pxBle, &inChar, 5);
//						if (result == false)
//							break;
//					}
					READY_TO_RCV
					;
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
			} else if (outBuffer[0] == 0x07 && outBuffer[1] == 0x03)
			{
				hello_ble();
			} else
			{
				// send anything else that comes in on the ble queue
				for (int i = 1; i < size;)
				{
					//ERIC: Do we need to toggle HW flow control here? CTS off and ensure RTS is ready?
					xSerialPutChar(pxBle, outBuffer[i++], 5);
				}
			}
		}
	}
}

void reset_transmit()
{
	signed char inChar;

	NOT_READY_TO_RCV;
	vTaskDelay(10);
	for (;;)
	{
		result = xSerialGetChar(pxBle, &inChar, 5);
		if (result == false)
			break;
	}
	READY_TO_RCV
}

void hello_ble()
{
	ble_usart_tx(pxBle, helloCmd, 4);
}

uint8_t discoverParams2[] = { 0x00, 0x05, 0x06, 0x07, 0x40, 0x00, 0x32, 0x00, 0x00 };

void config_ble()
{
	ble_usart_tx(pxBle, endDiscoverCmd, 4);
//	vTaskDelay(50);
	//ble_usart_tx(pxBle, discoverParams2, 9);
//	vTaskDelay(50);
//	ble_usart_tx(pxBle, discoverCmd, 5);
//	vTaskDelay(50);
//	ble_usart_tx(pxBle, modeCmd, 6);
//	vTaskDelay(50);
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
//		led_alert_on();
	} else
	{
		ble_queue_created = true;
		xTaskCreate(task_ble, "ble", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);
		xTaskCreate(task_ble_tx, "ble_tx", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);
	}
}
