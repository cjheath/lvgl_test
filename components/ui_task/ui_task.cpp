/*
 * LVGL GUI Display
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#ifndef LV_HOR_RES_MAX
#define LV_HOR_RES_MAX	CONFIG_LV_HOR_RES_MAX
#endif // LV_HOR_RES_MAX
#ifndef LV_VER_RES_MAX
#define LV_VER_RES_MAX	CONFIG_LV_VER_RES_MAX
#endif // LV_VER_RES_MAX
#ifndef	LV_TICK_PERIOD_MS
#define	LV_TICK_PERIOD_MS	1
#endif	// LV_TICK_PERIOD_MS

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "ui_task.h"

static void		ui_task(void *pvParameter);

using namespace LVGL;

// Initialise a display
Display::Display()
: display(0)
, display_driver((lv_disp_drv_t*)malloc(sizeof(lv_disp_drv_t)))
, gui_semaphore(xSemaphoreCreateMutex())
, buf1((lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA))
, buf2(0)
, draw_buf((lv_disp_draw_buf_t*)malloc(sizeof(lv_disp_draw_buf_t)))
, input_driver(0)
, periodic_timer(0)
{
	// Initialise the LVGL library
	lv_init();

	// Initialize the LVGL device driver (SPI or I2C bus, etc)
	lvgl_driver_init();

	init_driver();

	init_pointer_device();
	display = lv_disp_get_default();

	run();
}

Display::~Display()
{
	// REVISIT: GUI task never exits so this can do nothing useful
}

// Call this lambda under control of the display semaphore
void
Display::synchronised(void (*lambda)())
{
	if (pdTRUE == xSemaphoreTake(gui_semaphore, portMAX_DELAY))
	{
		lv_disp_set_default(display);		// Ensure correct display is default
		lambda();
		xSemaphoreGive(gui_semaphore);
	}
}

void Display::init_driver()
{
	assert(buf1 != NULL);	// allocated in the constructor

	// Use double buffering except with monochrome displays
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf2 != NULL);
#endif

	uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

	// Actual size in pixels, not bytes
	size_in_px *= 8;
#endif

	// Initialize the working buffers
	lv_disp_draw_buf_init(draw_buf, buf1, buf2, size_in_px);

	assert(display_driver != 0);	// Allocated in the constructor
	lv_disp_drv_init(display_driver);
	display_driver->flush_cb = disp_driver_flush;

	// For monochrome displays, register callbacks to adjust transfer sizes to whole bytes
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	display_driver->rounder_cb = disp_driver_rounder;
	display_driver->set_px_cb = disp_driver_set_px;
#endif

	display_driver->draw_buf = draw_buf;
	lv_disp_drv_register(display_driver);
}

void Display::init_pointer_device()
{
	// Register an input device when enabled on the menuconfig
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
	input_driver = (lv_indev_drv_t*)malloc(sizeof(lv_indev_drv_t));
	assert(input_driver != 0);
	lv_indev_drv_init(input_driver);
	input_driver->read_cb = touch_driver_read;
	input_driver->type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(input_driver);
#endif
}

void Display::start_ui_timer()
{
	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = [](void*) { lv_tick_inc(LV_TICK_PERIOD_MS); },
		.arg = 0,
		//.dispatch_method = ESP_TIMER_ISR,	// Not available in this version of ESP-IDF
		.dispatch_method = ESP_TIMER_TASK,
		.name = "periodic_gui"
	};
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

void Display::run()
{
	start_ui_timer();

	// NOTE: When not using Wi-Fi nor Bluetooth you could pin the ui_task to core 0
	xTaskCreatePinnedToCore(ui_task, "gui", 4096*2, (void*)this, 0, NULL, 1);
}

static void ui_task(void *pvParameter)
{
	Display*	self = (Display*)pvParameter;

	while (1)
	{
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		self->synchronised([]() {
			lv_task_handler();
		});
	}
	// Unreachable
}
