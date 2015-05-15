/*
 * ble_serial.c
 *
 *  Created on: Feb 19, 2015
 *      Author: titan
 */

#include <stdio.h>
#include "task_ble_serial.h"
#include "task_ble_dispatch.h"

#define BUFFER_MAX		50
#define BUFFER_SIZE     (BUFFER_MAX + 1)
#define QUEUE_SIZE		10
#define QUEUE_TICKS		10

static signed char inBuffer[BUFFER_MAX + 1];
static int bufferIndex;
static uint8_t command = 0;

static portTASK_FUNCTION(task_ble, params)
{

	BaseType_t result;
	signed char inChar;
	xComPortHandle pxIn;

	pxIn = xSerialPortInitMinimal(1, 38400, 50); // ble

//	xComPortHandle pxOut;
//	pxOut = xSerialPortInitMinimal(0, 38400, 64); //WAN

	bufferIndex = 0;

	DDRD |= _BV(PD5);		// CTS - output
	DDRD &= ~_BV(PD4);		// RTS - input

	PORTD &= ~_BV(PD5);		// lower CTS to start the BLE stream

	for (;;)
	{
		result = xSerialGetChar(pxIn, &inChar, 0xFFFF);

		if (result == pdTRUE )
		{

			if (inChar == 'P')
			{
				// provision
				command = 1;
				inBuffer[bufferIndex++] = inChar;
			} else if (inChar == '*')
			{
				// regular broadcast
				command = 2;
				inBuffer[bufferIndex++] = inChar;
			} else if (inChar == '\n')
			{
				// we have a complete message
				// check what type of message we have
				switch (command)
				{
				case 1:
					result = xQueueSendToBack( xDispatchQueue, inBuffer, 0);
					bufferIndex = 0;
					command = 0;

					break;
				case 2:
						result = xQueueSendToBack( xDispatchQueue, inBuffer, 0);

					bufferIndex = 0;
					if (result != pdTRUE )
					{
						//led_alert_on();
					}
					command = 0;
					break;
				}
			} else
			{
				inBuffer[bufferIndex++] = inChar;
				if (bufferIndex >= BUFFER_MAX)
				{
					//led_alert_on();
					bufferIndex = 0;
				}
			}
		}
	}
}

void task_ble_serial_start(UBaseType_t uxPriority)
{

	xTaskCreate(task_ble, "monitor", configMINIMAL_STACK_SIZE, NULL, uxPriority, ( TaskHandle_t * ) NULL);

}
