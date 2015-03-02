/*
 * wan_msg.h
 *
 *  Created on: Feb 23, 2015
 *      Author: titan
 */

#ifndef WAN_MSG_H_
#define WAN_MSG_H_

#include "../sys/sysTypes.h"

typedef struct
{
	uint8_t		rssi;
	uint64_t	mac;
	uint16_t	batt;
	uint16_t	temp;

} wan_msg_t;

typedef struct
{
	uint8_t command;
}cmd_header_t;

typedef struct
{
	uint8_t command;
	uint16_t pan_id;
	uint16_t short_id;
	uint8_t channel;
}cmd_config_ntw_t;

enum
{
	CMD_SEND = 0x01,
	CMD_ACK_SEND = 0x02,
	CMD_CONFIG_NETWORK = 0x03,
	CMD_GET_ADDRESS = 0x04,
	CMD_IN_PROX = 0x05,
	CMD_OUT_PROX = 0x06

};

typedef struct PACK app_msg_t {
	uint8_t messageType;
	uint8_t nodeType;
	uint64_t extAddr;
	uint16_t shortAddr;
	uint64_t routerAddr;
	uint16_t panId;
	uint8_t workingChannel;
	uint16_t parentShortAddr;
	uint8_t lqi;
	int8_t rssi;
	uint8_t ackByte;

	int32_t battery;
	int32_t temperature;

	uint8_t cs;

} app_msg_t;

#endif /* WAN_MSG_H_ */
