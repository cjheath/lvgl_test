/*
 * LVGL user interface gui Task
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

extern "C" {
#include "lvgl.h"
#include "lvgl_helpers.h"
}

/*
 * A semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore!
 */
SemaphoreHandle_t xGuiSemaphore;

static void	init_semaphore();
static void	init_display_buffers();
static void	init_pointer_device();
static void	start_gui_timer();
static void	run_gui_task();
static void	gui_task(void *pvParameter);

// Initialise the display and process user interface events
esp_err_t gui_task_create()
{
	init_semaphore();

	// Initialise the LVGL library
	lv_init();

	// Initialize the LVGL device driver (SPI or I2C bus, etc)
	lvgl_driver_init();

	init_display_buffers();

	init_pointer_device();

	start_gui_timer();

	// Start the thread to process GUI events
	run_gui_task();

	return 0;
}

static void	init_semaphore()
{
	xGuiSemaphore = xSemaphoreCreateMutex();
}

static void init_display_buffers()
{
	lv_color_t*	buf1;
	lv_color_t*	buf2 = 0;

	buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf1 != NULL);

	// Use double buffering except with monochrome displays
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf2 != NULL);
#endif

	static lv_disp_draw_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
	|| defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

	// Actual size in pixels, not bytes
	size_in_px *= 8;
#endif

	// Initialize the working buffers
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;

	// For monochrome displays, register callbacks to adjust transfer sizes to whole bytes
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	disp_drv.rounder_cb = disp_driver_rounder;
	disp_drv.set_px_cb = disp_driver_set_px;
#endif

	disp_drv.draw_buf = &disp_buf;
	lv_disp_drv_register(&disp_drv);
}

static void init_pointer_device()
{
	// Register an input device when enabled on the menuconfig
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);
#endif
}

static void start_gui_timer()
{
	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = [](void*) { lv_tick_inc(LV_TICK_PERIOD_MS); },
		.arg = 0,
		//.dispatch_method = ESP_TIMER_ISR,	// Not available in this version of ESP-IDF
		.dispatch_method = ESP_TIMER_TASK,
		.name = "periodic_gui"
	};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

static void run_gui_task()
{
	// NOTE: When not using Wi-Fi nor Bluetooth you could pin the gui_task to core 0
	xTaskCreatePinnedToCore(gui_task, "gui", 4096*2, NULL, 0, NULL, 1);
}

static void gui_task(void *pvParameter)
{
	(void) pvParameter;

	while (1)
	{
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		// Wait to take the semaphore then process lvgl tasks
		if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
		{
			lv_task_handler();
			xSemaphoreGive(xGuiSemaphore);
		}
	}
	// Unreachable
}
