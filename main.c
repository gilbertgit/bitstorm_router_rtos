#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Application include files. */
#include "task_blinky.h"
#include "task_serial_test.h"
#include "task_adc_test.h"
#include "ble/task_ble_serial.h"
#include "ble/task_ble_dispatch.h"
#include "ble/task_ble_monitor.h"
#include "wan/wan_task.h"
#include "util/clock.h"
#include "ramdisk/ramdisk.h"
#include "util/task_monitor.h"

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
    MCUSR = 0; // clear reset flags
    wdt_disable();
}

int main(void)
{

	/////////////////////
	_delay_ms(1500); // This is for development only. The MRKII programmer will reset the chip right after boot up.
	/////////////////////


	// PB0 is used for CTS of message
	DDRB &= ~(1 << PB0);	// SET PB0 TO INPUT
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT8);

	// PB1 is used for configure
	DDRB &= ~(1 << PB1); //INPUT
	PORTB |= (1 << PB1);
	clock_init();
	ramdisk_init();
	sei();

	task_blinky_start((tskIDLE_PRIORITY + 1));
	task_ble_monitor_start(tskIDLE_PRIORITY + 1);

	task_ble_dispatch_start(tskIDLE_PRIORITY + 1);
//	task_wan_start(tskIDLE_PRIORITY + 1);
	task_adc_test_start(tskIDLE_PRIORITY + 1);

	task_ble_serial_start(tskIDLE_PRIORITY + 1);

//	task_monitor_start(tskIDLE_PRIORITY + 1);

	vTaskStartScheduler();

	// Never get here please
	return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	led_alert_on();
}
/*-----------------------------------------------------------*/

