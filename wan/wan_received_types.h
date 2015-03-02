/*
 * wan_recieved_types.h
 *
 *  Created on: Dec 5, 2014
 *      Author: titan
 */

#ifndef WAN_RECIEVED_TYPES_H_
#define WAN_RECIEVED_TYPES_H_

typedef struct mac_resp_t
{
	uint8_t request_type;
	uint64_t wan_device_address;

} mac_resp_t;

typedef struct config_ntw_resp_t
{
	uint8_t request_type;
	uint8_t resp;

} config_ntw_resp_t;

typedef struct no_ack_status_resp_t
{
	uint8_t request_type;
	uint8_t resp;

} no_ack_status_resp_t;

#endif /* WAN_RECIEVED_TYPES_H_ */
