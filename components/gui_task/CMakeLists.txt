set(gui_task_srcs "src/gui_task.cpp")

idf_component_register(
		SRCS "${gui_task_srcs}"
		INCLUDE_DIRS "include"
		REQUIRES lvgl lvgl_esp32_drivers
	)
