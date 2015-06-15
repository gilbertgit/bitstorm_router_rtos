/*
 * task_spi_test.c

 *
 *  Created on: May 18, 2015
 *      Author: titan
 */
#include <stdio.h>

#include "task_spi_test.h"
#include "led.h"
#include "spi.h"
#define ACK 0x7E

TaskHandle_t xSPITaskHandle;
static xComPortHandle pxSpi;
static char buffer[10];

const static TickType_t xDelay = 2 / portTICK_PERIOD_MS;

static portTASK_FUNCTION(task_spi, pvParameters)
{
	init_spi_master();
	pxSpi = xSerialPortInitMinimal(0, 38400, 50);

	unsigned char data; //Received data stored here

	buffer[0] = 'S';
	buffer[1] = 'P';
	buffer[2] = 'I';
	buffer[3] = ':';
	buffer[4] = ' ';
	buffer[5] = 'O';
	buffer[6] = 'K';
	buffer[7] = '\n';

	for (;;)
	{
		//vTaskDelay(xDelay);
		//sprintf(buffer, "SPI: OK");
		for (int i = 0; i < 9; i++)
		{
			if (send_spi_msg == 0x01)
				data = spi_tranceiver(buffer[i]);     //Send "x", receive ACK in "data"
			vTaskDelay(xDelay);
		}

//		if (data == ACK)
//		{
//			led_alert_toggle();
//			sprintf(buffer, "SPI: OK");
//			for (int i = 0; buffer[i]; i++)
//				xSerialPutChar(pxSpi, buffer[i], 0);
//		}
//		else
//			xSerialPutChar(pxSpi, data, 0);
	}

}

void task_spi_start(UBaseType_t uxPriority)
{
	xTaskCreate(task_spi, "task_spi", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xSPITaskHandle);

}
