/*
 * router_status_task.c
 *
 *  Created on: May 4, 2015
 *      Author: titan
 */
#include "router_status_task.h"
#include "router_status_msg.h"
#include "../wan/wan_task.h"
#include "../shared.h"

const static TickType_t xDelay = 5000 / portTICK_PERIOD_MS;

uint16_t adc_result;
static router_status_msg_t msg;
static BaseType_t result;
static router_msg_t router_msg;

QueueHandle_t xDispatchQueue;
TaskHandle_t xRouterStatusHandle;

volatile unsigned char resetReason __attribute__ ((section (".noinit")));

static portTASK_FUNCTION(task_router_status, params)
{
	initADC();
	for (;;)
	{
		vTaskDelay(xDelay);

//		msg.type = 8;
//		msg.reset_source = resetReason;
//		msg.adc = fetch_ADC();
//		msg.reset_cause = reset_cause.cause;
//		result = xQueueSendToBack( xWANQueue, &msg, 0);

		router_msg.messageType = 0x08;
		router_msg.routerReset = resetReason;
		router_msg.resetTask = reset_cause.cause;
		result = xQueueSendToBack( xWANQueue, &router_msg, 0);
	}
}

void initADC()
{
	// Setup ADC read on PA0 - V_CHG
	ADMUX = (0 << REFS1) | (0 << REFS0);					// AVCC, connected to 3V regulator
	ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 < ADPS0);		// Prescale 128
}

uint16_t fetch_ADC()
{
	// Start ADC conversion
	ADCSRA |= (1 << ADEN) | (1 << ADSC);
	// Wait for completion
	//ERIC: Can this turn into an infinite loop?
	while ((ADCSRA & (1 << ADSC)) == (1 << ADSC))
		;
	// Get results
	adc_result = ADC;
	// Disable ADC to save power
	ADCSRA &= ~(1 << ADEN);

	return adc_result;
}

void task_router_status_start(UBaseType_t uxPriority)
{
	xTaskCreate(task_router_status, "router_status", configMINIMAL_STACK_SIZE, NULL, uxPriority, &xRouterStatusHandle);
}
