/*
 * task_wan.h
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#ifndef TASK_WAN_H_
#define TASK_WAN_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"
#include "led.h"

#include "../ble/task_ble_dispatch.h"
#include "wan_msg.h"
#include "../util/router_status_msg.h"

#define NWK_READY (PINB & (1 << PB0))
#define NWK_BUSY  (~(PINB & (1 << PB0)))
#define NWK_CONFIG !(PINB & (1 << PB1))
#define WAN_BUSY 0x01

extern QueueHandle_t xWANQueue;
extern TaskHandle_t xWanTaskHandle;
extern uint16_t xWanMonitorCounter;
extern xComPortHandle pxWan;

void task_wan_start(UBaseType_t uxPriority);
void send_router_status_msg(xComPortHandle hnd, router_status_msg_t *msg);
void sendMessage(xComPortHandle hnd, btle_msg_t *msg);
void build_app_msg(btle_msg_t *btle_msg, app_msg_t *msg);
void wan_state_configure(void);
void waitForResp();
void configure_wan();
//void synchronize();
void waitForNwkConfigResp();
void waitForNwkConfigResp2();
void decode_cobs(const unsigned char *ptr, unsigned long length, unsigned char *dst);
void update_changeset();

#endif /* TASK_WAN_H_ */
