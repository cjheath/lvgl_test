/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include "gui_task.h"
#include "wifi_lvgl.h"

extern "C"{
	void app_main();
}

void app_main()
{
	esp_err_t	err = gui_task_create();

        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
	{
		create_wifi_provisioning_ui();
		xSemaphoreGive(xGuiSemaphore);
	}
	
	// REVISIT: Rest of the app to be added here
}
