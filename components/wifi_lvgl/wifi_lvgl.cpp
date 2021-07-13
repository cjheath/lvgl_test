/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include "gui_task.h"

void create_wifi_provisioning_ui(void)
{
	// REVISIT: Not Yet Implemented. Just display a message box
	static const char * btns[] = {"Cancel", "Ok", ""};

	lv_obj_t* m = lv_msgbox_create(lv_scr_act(), "It worked", "Press ok to acknowledge", btns, true);
	lv_obj_align(m, LV_ALIGN_CENTER, 0, 0);
}
