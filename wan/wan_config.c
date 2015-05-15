/*
 * wan_config.c
 *
 *  Created on: Dec 4, 2014
 *      Author: titan
 */
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "wan_msg.h"
#include "wan_config.h"
#include "wan_received_types.h"
#include "../shared.h"

shared_t shared;
static uint8_t resp_type;

mac_resp_t mac_resp;

enum {
	resp_type_config_ntw = 0x03, resp_type_address = 0x04,
};
enum states {
	SEND_MAC_REQ, CONFIG_NTW_REQ, AWAITING_RESP, FINISHED
};
uint8_t state = SEND_MAC_REQ;

void wan_get_device_address()
{
	xComPortHandle pxOut;
	pxOut = xSerialPortInitMinimal(0, 38400, 10);
	cmd_header_t cmd_header;
	uint8_t frame[10];
	frame[0] = sizeof(cmd_header) + 1;
	cmd_header.command = CMD_GET_ADDRESS;
	int frame_index = 1;
	// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
	// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= frame[i++])
		;
	frame[frame_index++] = cs;

	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(pxOut, frame[i++], 5);
	}
}

void wan_config_network()
{
	xComPortHandle pxOut;
	pxOut = xSerialPortInitMinimal(0, 38400, 10);
	cmd_config_ntw_t config_ntw;

	config_ntw.command = CMD_CONFIG_NETWORK;
	config_ntw.pan_id = router_config.pan_id;
	config_ntw.short_id = router_config.mac & 0x0000FFFF;
	config_ntw.channel = router_config.channel;

	uint8_t frame[10];
	frame[0] = sizeof(config_ntw) + 1; // size of message

	int frame_index = 1;
	//config
	for (int i = 0; i < sizeof(config_ntw); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&config_ntw))[i];
	}
	// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= frame[i++])
		;
	frame[frame_index++] = cs;

	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(pxOut, frame[i++], 5);
	}
}

void wan_config_done()
{
	xComPortHandle pxOut;
	pxOut = xSerialPortInitMinimal(0, 38400, 10);

	cmd_header_t cmd_header;
	uint8_t frame[10];
	frame[0] = sizeof(cmd_header) + 1;
	cmd_header.command = CMD_CONFIG_DONE;
	int frame_index = 1;

	// header
	for (int i = 0; i < sizeof(cmd_header); i++)
	{
		frame[frame_index++] = ((uint8_t *) (&cmd_header))[i];
	}
	// checksum
	uint8_t cs = 0;
	for (int i = 0; i < frame_index; cs ^= frame[i++])
		;
	frame[frame_index++] = cs;

	for (int i = 0; i < frame_index;)
	{
		xSerialPutChar(pxOut, frame[i++], 5);
	}
}

bool wan_config_received(uint8_t * buff)
{
	resp_type = buff[1];
	switch (resp_type)
	{
	case resp_type_address:
		config_mac_resp((mac_resp_t *) &buff[1]);
		break;
	case resp_type_config_ntw:
		config_ntw_resp((config_ntw_resp_t *) &buff[1]);
		break;
	}

	return (state == FINISHED ? true : false);
}

void config_mac_resp(mac_resp_t * resp)
{
	//shared.mac = resp->wan_device_address;
	state = CONFIG_NTW_REQ;
}

void config_ntw_resp(config_ntw_resp_t * resp)
{
	state = FINISHED;
}

void no_ack_status_resp(no_ack_status_resp_t * resp)
{
	state = FINISHED;
}

bool wan_config()
{
	switch (state)
	{
	case SEND_MAC_REQ:
		wan_get_device_address();
		state = AWAITING_RESP;
		break;
	case CONFIG_NTW_REQ:
		wan_config_network();
		state = AWAITING_RESP;
	}
	return (state == FINISHED ? true : false);
}
