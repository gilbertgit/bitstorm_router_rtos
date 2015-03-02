
#include <stdlib.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Application include files. */
#include "task_blinky.h"
#include "task_serial_test.h"
#include "ble/task_ble_serial.h"
#include "ble/task_ble_dispatch.h"
#include "ble/task_ble_monitor.h"
#include "wan/wan_task.h"
#include "util/clock.h"
#include "ramdisk/ramdisk.h"

/*-----------------------------------------------------------*/

int main(void) {

	clock_init();
	ramdisk_init();
	sei();
	task_blinky_start( (tskIDLE_PRIORITY+1) );
	//task_serial_test_start( (tskIDLE_PRIORITY+2) );

	task_ble_monitor_start(tskIDLE_PRIORITY+2);
	task_wan_start(tskIDLE_PRIORITY+3);
	task_ble_dispatch_start(tskIDLE_PRIORITY+4);
	task_ble_serial_start(tskIDLE_PRIORITY+5);
	vTaskStartScheduler();

	while(1)
	{

	}

	// Never get here please
	return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName ) {
	led_alert_on();
}
/*-----------------------------------------------------------*/

