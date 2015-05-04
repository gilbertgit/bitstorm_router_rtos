/*
 * task_adc_test.h
 *
 *  Created on: May 4, 2015
 *      Author: ericrudisill
 */

#ifndef INCLUDE_TASK_ADC_TEST_H_
#define INCLUDE_TASK_ADC_TEST_H_

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "serial.h"

void task_adc_test_start( UBaseType_t uxPriority );

#endif /* INCLUDE_TASK_ADC_TEST_H_ */
