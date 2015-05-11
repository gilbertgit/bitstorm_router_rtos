/*
 * router_status_task.h
 *
 *  Created on: May 4, 2015
 *      Author: titan
 */

#ifndef ROUTER_STATUS_TASK_H_
#define ROUTER_STATUS_TASK_H_

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

extern volatile unsigned char resetReason __attribute__ ((section (".noinit")));

void initADC();
void fetch_MCUSR();
uint16_t fetch_ADC();
void task_router_status_start(UBaseType_t uxPriority);

#endif /* ROUTER_STATUS_TASK_H_ */
