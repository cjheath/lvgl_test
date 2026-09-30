set(ui_task_srcs "ui_task.cpp")

idf_component_register(
		SRCS "${ui_task_srcs}"
		INCLUDE_DIRS "."
		REQUIRES lvgl
		PRIV_REQUIRES lvgl_touch_calibration esp_lcd_ili9341 esp_lcd_touch_xpt2046 esp_lvgl_port driver esp_lcd nvs_flash
	)
