/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include "sdkconfig.h"

#if !defined(CONFIG_APP_MAIN_CONSOLE_SCAN)

#include "ui_task.h"
#include "thread.h"

extern "C"{
	void app_main();
}

using namespace LVGL;

void app_main()
{
	new Display();

	MainThread	main_thread;
	for (;;)
		Thread::yield(Milliseconds(1000));
}

#endif	// !CONFIG_APP_MAIN_CONSOLE_SCAN
