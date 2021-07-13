idf_component_register(
		SRCS "wifi_lvgl.cpp"
		INCLUDE_DIRS "."
		REQUIRES gui_task lvgl lvgl_esp32_drivers
	)
