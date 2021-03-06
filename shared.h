/*
 * shared.h
 *
 *  Created on: Dec 2, 2014
 *      Author: titan
 */

#ifndef SHARED_H_
#define SHARED_H_

#define TRACE(x)	reset_cause.trace = x;

enum reset_causes
{
WAN_TASK_M = 0x01, BLE_TASK_M = 0x02, BLE_DISPATCH_TASK_M = 0x03, WAN_MSG_RT = 0x04, WAN_CONFIG_RT = 0x05, WAN_CFG = 0x06, WAN_DISPATCH_TASK_M = 0x07
};

typedef struct
{
	uint64_t mac;

}shared_t;

typedef struct
{
	uint8_t magic;
	uint64_t mac;
	uint16_t pan_id;
	uint8_t channel;
}router_config_t;

typedef struct
{
	uint8_t cause;
	uint16_t trace;
}reset_cause_t;

typedef struct
{
	uint8_t id;
}changeset_t;

typedef struct
{
	uint8_t threshold;
}rssi_threshold_t;

extern shared_t shared;
extern router_config_t router_config;
extern reset_cause_t reset_cause;
extern changeset_t changeset;
extern uint32_t my_tick_count;
extern uint8_t sync_count;
extern rssi_threshold_t rssi_threshold;

void read_config();
void write_config();
void read_reset_cause();
void write_reset_cause();
void read_rssi_threshold();
void write_rssi_threshold();

#endif /* SHARED_H_ */
