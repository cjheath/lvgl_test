/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include "ui_task.h"
#include "wifi_credential_ui.h"
#include "lvgl_touchscreen_cal.h"
#include "freertos/task.h"

extern "C"{
	void app_main();
}

using namespace LVGL;

void app_main()
{
	Display*	display = new Display();

	display->synchronised([]() {
		create_wifi_credential_ui();
		// touchscreen_cal_create();
	});

	// REVISIT: Rest of the app to be added here
	// display->run();

	vTaskDelay(portMAX_DELAY);
}
