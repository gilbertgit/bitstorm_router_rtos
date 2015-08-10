#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Application include files. */
#include "task_blinky.h"
//#include "task_serial_test.h"
//#include "task_adc_test.h"
#include "ble/task_ble_serial.h"
#include "ble/task_ble_dispatch.h"
#include "ble/task_ble_monitor.h"
#include "wan/wan_task.h"
#include "wan/task_wan_dispatch.h"
#include "util/clock.h"
#include "ramdisk/ramdisk.h"
#include "util/task_monitor.h"
#include "util/router_status_task.h"
//#include "task_spi_test.h"
#include "wan.h"
#include "ble.h"

/*-----------------------------------------------------------*/

// This function is called upon a HARDWARE RESET:
void reset(void) __attribute__((naked)) __attribute__((section(".init3")));

/*! Clear SREG_I on hardware reset. */
//ERIC: Are we sure this is still getting invoked? Could explain the odd behavior of it resetting but then hanging.
//ERIC: Also, additional details here: http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
//ERIC: Also, additional information here:http://www.atmel.com/images/doc2551.pdf
//ERIC: Also, are fuses set correctly?
void reset(void)
{
	cli();
	// Note that for newer devices (any AVR that has the option to also
	// generate WDT interrupts), the watchdog timer remains active even
	// after a system reset (except a power-on condition), using the fastest
	// prescaler value (approximately 15 ms). It is therefore required
	// to turn off the watchdog early during program startup.
	resetReason = MCUSR;
	MCUSR = 0; // clear reset flags
	wdt_disable();
}

int main(void)
{
	/////////////////////
	_delay_ms(1500); // This is for development only. The MRKII programmer will reset the chip right after boot up.
	/////////////////////


	//ERIC: Where are we bouncing the two chips, wan and ble?? Shouldn't this happen at 1284 startup? Or at least in their startup tasks?
	kill_wan();
	_delay_ms(500);
	init_wan(); // GE: for some reason I can't do this inside the task..
	kill_ble();	// GE: I'm doing the init in the ble serial task

	clock_init();
	ramdisk_init();
	sei();

	//task_ble_monitor_start(tskIDLE_PRIORITY + 1);

	task_ble_dispatch_start(tskIDLE_PRIORITY + 1);

	task_wan_start(tskIDLE_PRIORITY + 1);

	task_wan_dispatch_start(tskIDLE_PRIORITY + 1);

	task_ble_serial_start(tskIDLE_PRIORITY + 1);

	task_blinky_start((tskIDLE_PRIORITY + 1));

	task_monitor_start(tskIDLE_PRIORITY + 1);

	task_router_status_start(tskIDLE_PRIORITY + 1);

	vTaskStartScheduler();

	// Never get here please
	return 0;
}

//extern xComPortHandle pxWan;

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	led_alert_on();
//	xSerialPutChar(pxWan, 0xCC, 5);
//	xSerialPutChar(pxWan, 0xCC, 5);
//	for (int i=0;i<10;i++)
//		xSerialPutChar(pxWan, pcTaskName[i], 5);
}
/*-----------------------------------------------------------*/

