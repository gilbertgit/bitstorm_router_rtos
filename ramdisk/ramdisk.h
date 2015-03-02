/*
 * ramdisk.h
 *
 *  Created on: Nov 26, 2014
 *      Author: Will
 */

#ifndef RAMDISK_RAMDISK_H_
#define RAMDISK_RAMDISK_H_

#include "../ble/ble_msg.h"

#define SIZE_OF 25

void ramdisk_init();
int ramdisk_write(btle_msg_t to_write);
int ramdisk_erase(btle_msg_t to_remove);
btle_msg_t *ramdisk_find(uint64_t target);
btle_msg_t *ramdisk_next(btle_msg_t *target);


#endif /* RAMDISK_RAMDISK_H_ */
