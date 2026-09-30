idf_component_register(
		SRCS "wifi_credential_ui.cpp"
		INCLUDE_DIRS "."
		REQUIRES ui_task lvgl
		PRIV_REQUIRES esp_wifi esp_event nvs_flash
	)
