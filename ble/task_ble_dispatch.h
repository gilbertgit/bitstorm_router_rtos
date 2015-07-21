/*
 * task_ble_dispatch.h
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#ifndef TASK_BLE_DISPATCH_H_
#define TASK_BLE_DISPATCH_H_

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"
#include "ble_msg.h"

#include "../shared.h"

extern QueueHandle_t xDispatchQueue;

#define MSG_TYPE_NORM 		1
#define MSG_TYPE_IN_PROX 	5
#define MSG_TYPE_OUT_PROX 	6

void task_ble_dispatch_start( UBaseType_t uxPriority );
//bool btle_handle_le_packet(char * buffer, btle_msg_t *);
uint8_t btle_parse_nybble(char c);

void read_config();
void write_config();
void read_changeset();

//bool handle_router_config_packet(char * buffer, router_config_t * conf);
bool handle_router_config_packet2(char * buffer, router_config_t * conf);

bool btle_handle_le_packet2(char * buffer, btle_msg_t * btle_msg);
void system_class();
void attr_db_class();
void gap_class();
void connection_class();

#endif /* TASK_BLE_DISPATCH_H_ */
