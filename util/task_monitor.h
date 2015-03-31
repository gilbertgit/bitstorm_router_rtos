/*
 * task_monitor.h
 *
 *  Created on: Mar 26, 2015
 *      Author: titan
 */

#ifndef TASK_MONITOR_H_
#define TASK_MONITOR_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

void task_monitor_start(UBaseType_t uxPriority);
void reboot_1284();

#endif /* TASK_MONITOR_H_ */
