set(srcs "main.cpp")

idf_component_register(
		SRCS ${srcs}
		INCLUDE_DIRS "."
		PRIV_REQUIRES ui_task strpp
	)

