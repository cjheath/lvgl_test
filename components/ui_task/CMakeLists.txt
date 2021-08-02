set(ui_task_srcs "ui_task.cpp")

idf_component_register(
		SRCS "${ui_task_srcs}"
		INCLUDE_DIRS "."
		REQUIRES lvgl lvgl_esp32_drivers
	)
