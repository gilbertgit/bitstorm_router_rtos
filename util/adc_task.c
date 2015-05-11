/*
 * adc_task.c
 *
 *  Created on: May 4, 2015
 *      Author: titan
 */

#include <stdio.h>
#include "adc_task.h"

static xComPortHandle pxAdc;

static char buffer[40];

const static TickType_t xDelay = 1000 / portTICK_PERIOD_MS;

TaskHandle_t xADCTaskHandle;

static portTASK_FUNCTION(task_adc, params)
{
	uint16_t result;
	uint32_t milliVolts;
	pxAdc = xSerialPortInitMinimal(0, 38400, 50);

	// Setup ADC read on PA0 - V_CHG
	ADMUX = (0 << REFS1) | (0 << REFS0);					// AVCC, connected to 3V regulator
	ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 < ADPS0);		// Prescale 128


	for (;;)
	{
		vTaskDelay(xDelay);

		// Start ADC conversion
		ADCSRA |= (1 << ADEN) | (1 << ADSC);
		// Wait for completion
		while ( (ADCSRA & (1 << ADSC)) == (1 << ADSC) )	;
		// Get results
		result = ADC;
		// Disable ADC to save power
		ADCSRA &= ~(1 << ADEN);

		ADC_value = result;
		// TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST
		// Calculate voltage based on 3V reference and 50% voltage divider
		// For production, just send the raw ADC over the air, since we won't
		// know exactly how the incoming voltage will be scaled.  The raw ADC
		// value just represents a measurement between 1-3V.  How that translates
		// to the actual thing attached is dependent on other circuitry.
//		milliVolts = (3000 * (uint32_t)ADC) / 1024 * 2;
//		sprintf(buffer, "ADC: %02X   mV: %lu\r\n", result, milliVolts);
//		for (int i=0; buffer[i]; i++)
//			xSerialPutChar(pxAdc, buffer[i], 0);
		// TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST

	}

	void task_adc_start(UBaseType_t uxPriority)
	{
		xTaskCreate(task_adc, "adc", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xADCTaskHandle);
	}
}
