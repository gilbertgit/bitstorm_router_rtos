/*
 * adc_task.h
 *
 *  Created on: May 4, 2015
 *      Author: titan
 */

#ifndef ADC_TASK_H_
#define ADC_TASK_H_

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

extern uint16_t ADC_value;
extern TaskHandle_t xADCTaskHandle;

void task_adc_start(UBaseType_t uxPriority);

#endif /* ADC_TASK_H_ */
