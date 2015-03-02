// ramdisk.c

#include "ramdisk.h"
#include <stdlib.h>

static btle_msg_t records[SIZE_OF];

static btle_msg_t *valid_head;

static btle_msg_t *deleted_head;

void ramdisk_init()
{
	int i;

	for (i = 0; i < SIZE_OF - 1; i++)
	{
		records[i].next = &records[i + 1];
	}

	records[i].next = NULL;

	deleted_head = &records[0];

	valid_head = NULL;

}

int ramdisk_write(btle_msg_t to_write)
{
	btle_msg_t *temp_1;

	//Check for out of Memory
	if (deleted_head == NULL )
		return 0x00;

	//Check for no records
	if (valid_head == NULL )
	{
		temp_1 = deleted_head->next;
		valid_head = deleted_head;
		deleted_head = temp_1;
		valid_head->mac = to_write.mac;
		valid_head->batt = to_write.batt;
		valid_head->rssi = to_write.rssi;
		valid_head->temp = to_write.temp;
		valid_head->next = NULL;
		valid_head->last_sent = to_write.last_sent;
		valid_head->count = to_write.count;
		valid_head->type = to_write.type;
	}

	else
	{
		deleted_head->mac = to_write.mac;
		deleted_head->batt = to_write.batt;
		deleted_head->rssi = to_write.rssi;
		deleted_head->temp = to_write.temp;
		deleted_head->last_sent = to_write.last_sent;
		deleted_head->count = to_write.count;
		deleted_head->type = to_write.type;
		temp_1 = valid_head;
		valid_head = deleted_head;
		deleted_head = deleted_head->next;
		valid_head->next = temp_1;

	}

	return 0xff;

}

int ramdisk_erase(btle_msg_t to_remove)
{
	btle_msg_t *temp_1 = valid_head;
	//Move temp to target(to_remove);
	while (temp_1->mac != to_remove.mac && temp_1 != NULL )
	{
		temp_1 = temp_1->next;
	}

	//Handle Erase cases
	if (temp_1 == NULL )
		return 0x00;

	else if (temp_1 == valid_head)
	{
		valid_head = valid_head->next;
		temp_1->next = deleted_head;
		deleted_head = temp_1;
	}

	else
	{
		btle_msg_t *temp_2 = valid_head;
		while (temp_2->next != temp_1)
		{
			temp_2 = temp_2->next;
		}

		temp_2->next = temp_1->next;
		temp_2 = deleted_head;
		deleted_head = temp_1;
		deleted_head->next = temp_2;
	}

	return 0xff;
}

btle_msg_t * ramdisk_find(uint64_t target)
{
	btle_msg_t *temp_1 = valid_head;

	while (temp_1->mac != target && temp_1 != NULL )
	{
		temp_1 = temp_1->next;
	}

	return temp_1;
}

btle_msg_t * ramdisk_next(btle_msg_t * target)
{
	if (target == NULL )
		return valid_head;
	else
		return (target->next);
}

