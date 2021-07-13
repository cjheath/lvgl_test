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

#include "sdkconfig.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

// Actions on the created GUI must be synchronised under this semaphore
extern	SemaphoreHandle_t	xGuiSemaphore;

// Initialise the display and process user interface events
esp_err_t			gui_task_create();
