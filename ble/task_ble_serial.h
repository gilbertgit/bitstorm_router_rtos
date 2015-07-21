/*
 * ble_serial.h
 *
 *  Created on: Feb 19, 2015
 *      Author: titan
 */

#ifndef BLE_SERIAL_H_
#define BLE_SERIAL_H_

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

#include "../shared.h"

extern QueueHandle_t xBleQueue;
void config_ble();

void task_ble_serial_start( UBaseType_t uxPriority );

#endif /* BLE_SERIAL_H_ */
