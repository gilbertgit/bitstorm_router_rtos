/*
 * ble_msg.h
 *
 *  Created on: Feb 20, 2015
 *      Author: titan
 */

#ifndef BLE_MSG_H_
#define BLE_MSG_H_

#define MSG_SIZE				64 // index(1) + origin(11) + 1 + content(50) + 1 + type(1) + state(1) = 66

#include <stdint.h>

typedef struct {
	uint8_t type;
	uint8_t rssi;
	uint64_t mac;
	uint16_t batt;
	uint16_t temp;
	void *next;
	uint32_t last_sent;
	uint8_t count;
	uint16_t cs_id;
	uint16_t tagSerial;
	uint16_t tagStatus;

} btle_msg_t;

typedef struct {
	uint8_t command;
	uint16_t pan_id;
	uint16_t short_id;
	uint8_t message_length;
} cmd_send_header_t;

typedef struct pkg{
	uint8_t rssi;
	uint8_t packetType;
	uint64_t address;
//	uint8_t addressType;
//	uint8_t bond;
//	uint8_t dataLen;
//	uint8_t data[256];

} PACKAGE;

#endif /* BLE_MSG_H_ */
