set(srcs "main.cpp" "console_main.cpp")

idf_component_register(
		SRCS ${srcs}
		INCLUDE_DIRS "."
		PRIV_REQUIRES ui_task strpp wifi_scanner
	)
