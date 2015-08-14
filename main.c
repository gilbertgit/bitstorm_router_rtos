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

void init_wd()
{
	wdt_disable();
	WDTCSR |= (1 << WDCE) | (1 << WDE); // reset mode
	//WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0); // 64k Timeout
	wdt_enable(WDTO_2S);
}

void led_boot_sequence()
{
	led_init();
	led_green_on();
	led_alert_on();
	_delay_ms(50);
	led_green_off();
	led_alert_off();
	_delay_ms(50);
	led_green_on();
	led_alert_on();
	_delay_ms(50);
	led_green_off();
	led_alert_off();
}
void peripheral_boot_sequence()
{
	// kill wan
	WAN_DDR |= WAN_EN_bv; // OUTPUT
	WAN_PORT |= WAN_EN_bv;
	// kill ble
	BLE_DDR |= BLE_EN_bv; // OUTPUT
	BLE_PORT |= BLE_EN_bv;
	_delay_ms(100);

	// Enable WAN - Turn the ZB on
	WAN_DDR |= WAN_EN_bv; // OUTPUT
	WAN_PORT &= ~WAN_EN_bv; // LOW
	_delay_ms(100);

	// Enable BLE - Turn BG on
	BLE_DDR |= BLE_EN_bv; 	// OUTPUT
	BLE_PORT &= ~BLE_EN_bv; // LOW
	_delay_ms(100);
}

int main(void)
{
//	if (resetReason & (1 << 2))
//	{
//		led_init();
//		while (1)
//		{
//			_delay_ms(100);
//			led_alert_toggle();
//			wdt_reset();
//		}
//	}

	_delay_ms(100);

	led_boot_sequence();
	peripheral_boot_sequence();
	init_wd();

	clock_init();
	//ramdisk_init();
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
//	while (1)
//	{
//		wdt_reset();
//		_delay_ms(50);
//		led_green_on();
//		_delay_ms(50);
//		led_green_off();
//		_delay_ms(50);
//		led_green_on();
//		_delay_ms(50);
//		led_green_off();
//		_delay_ms(50);
//		led_green_on();
//
//		// kill wan
//		WAN_DDR |= WAN_EN_bv; // OUTPUT
//		WAN_PORT |= WAN_EN_bv;
//		// kill ble
//		BLE_DDR |= BLE_EN_bv; // OUTPUT
//		BLE_PORT |= BLE_EN_bv;
//		_delay_ms(100);
//
//		// Enable WAN - Turn the ZB on
//		WAN_DDR |= WAN_EN_bv; // OUTPUT
//		WAN_PORT &= ~WAN_EN_bv; // LOW
//		_delay_ms(100);
//
//		// Enable BLE - Turn BG on
//		BLE_DDR |= BLE_EN_bv; 	// OUTPUT
//		BLE_PORT &= ~BLE_EN_bv; // LOW
//		_delay_ms(1000);
//	}

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

