/*
 * task_ble_monitor.h
 *
 *  Created on: Feb 23, 2015
 *      Author: titan
 */

#ifndef TASK_BLE_MONITOR_H_
#define TASK_BLE_MONITOR_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

void task_ble_monitor_start( UBaseType_t uxPriority );
#endif /* TASK_BLE_MONITOR_H_ */
