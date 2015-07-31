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
#include "spi.h"

/*-----------------------------------------------------------*/

// This function is called upon a HARDWARE RESET:
void reset(void) __attribute__((naked)) __attribute__((section(".init3")));

/*! Clear SREG_I on hardware reset. */
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

void init_wd()
{
	wdt_disable();
	WDTCSR |= (1 << WDCE) | (1 << WDE); // reset mode
	WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0); // 64k Timeout
	wdt_enable(WDTO_2S);
}

int main(void)
{
	/////////////////////
	_delay_ms(1500); // This is for development only. The MRKII programmer will reset the chip right after boot up.
	/////////////////////

	///////////////////////////////////////////
	// Enable WAN
	init_wan();
	//_delay_ms(1000);
	// Enable BLE
#ifdef NEW_ROUTER
//	DDRC |= (1 << PC7); // OUTPUT
//	PORTC &= ~(1 << PC7); // LOW
//	_delay_ms(1000);
#endif
	///////////////////////////////////////////
	init_wd();
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

