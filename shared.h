/*
 * shared.h
 *
 *  Created on: Dec 2, 2014
 *      Author: titan
 */

#ifndef SHARED_H_
#define SHARED_H_

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

extern shared_t shared;
extern router_config_t router_config;

#endif /* SHARED_H_ */
