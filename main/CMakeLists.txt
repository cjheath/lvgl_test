set(srcs "main.cpp")

idf_component_register(
		SRCS ${srcs}
		INCLUDE_DIRS "."
		REQUIRES wifi_lvgl gui_task lvgl lvgl_esp32_drivers
	)

