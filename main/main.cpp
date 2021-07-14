/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include "gui_task.h"
#include "wifi_lvgl.h"
#include "freertos/task.h"

extern "C"{
	void app_main();
}

using namespace LVGL;

void app_main()
{
	Display*	display = new Display();

	display->synchronised([]() {
		create_wifi_provisioning_ui();
	});

	// REVISIT: Rest of the app to be added here
	// display->run();

	vTaskDelay(portMAX_DELAY);
}
