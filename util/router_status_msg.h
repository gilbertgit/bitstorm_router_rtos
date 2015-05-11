/*
 * router_status_msg.h
 *
 *  Created on: May 4, 2015
 *      Author: titan
 */

#ifndef ROUTER_STATUS_MSG_H_
#define ROUTER_STATUS_MSG_H_

typedef struct router_status_msg_t {
	uint8_t type;
	uint64_t router_address;
	uint16_t msg_sent_count;
	uint8_t reset_source;
	uint16_t adc;
}router_status_msg_t;

typedef struct {
	uint8_t command;
	//uint8_t message_length;
} cmd_router_status_header_t;

#endif /* ROUTER_STATUS_MSG_H_ */
