/*
 * task_spi_test.h
 *
 *  Created on: May 18, 2015
 *      Author: titan
 */

#ifndef TASK_SPI_TEST_H_
#define TASK_SPI_TEST_H_

#include "FreeRTOS.h"
#include "task.h"
#include "serial.h"
#include "spi.h"

void task_spi_start( UBaseType_t uxPriority );

#endif /* TASK_SPI_TEST_H_ */
