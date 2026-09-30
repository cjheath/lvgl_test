/*
 * LVGL user interface program to scan and display Wifi APs, select one, and enter+manage passwords
 */
#include	"ui_task.h"
#include	"lvgl.h"
#include	"lv_core/lv_obj.h"
#include	"wifi_credential_ui.h"

#include	"esp_wifi.h"
#include	"esp_log.h"
#include	"esp_event.h"


static	lv_obj_t*	wifi_scan_gui;
static	lv_obj_t*	keyboard_ui;

#define	TOP_HEIGHT	30	// Room for the title
#define	BOTTOM_HEIGHT	30	// Room for the ok/cancel buttons

static	void	show_wifi_credential_ui(bool show);
static	void	makeKeyboard();

/*
 * TEMPORARY: WiFi Scan inline
 */
#include "nvs_flash.h"
#define	DEFAULT_SCAN_LIST_SIZE	10
static	wifi_ap_record_t	ap_info[DEFAULT_SCAN_LIST_SIZE];
static	uint16_t		ap_count;
static	void			wifi_scan();
#define	LOG_TAG	"scan"

void create_wifi_credential_ui(LVGL::Display* ldp)
{
	lv_obj_t*	wifi_scan_list;
	lv_obj_t* cancel_btn;

	/*
	 * Get the Display and screen
	 */
	int screenWidth = ldp->display->driver.hor_res;
	int screenHeight = ldp->display->driver.ver_res;

	lv_obj_t*	screen = lv_scr_act();

	wifi_scan_gui = lv_obj_create(screen, NULL);
	lv_obj_set_pos(wifi_scan_gui, 0, 0);
	lv_obj_set_style_local_bg_color(wifi_scan_gui, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_obj_set_size(wifi_scan_gui, screenWidth, screenHeight);

	lv_obj_t*	wifi_scan_header = lv_label_create(wifi_scan_gui, NULL);
	lv_obj_set_size(wifi_scan_header, screenWidth, TOP_HEIGHT);
	lv_obj_align(wifi_scan_header, wifi_scan_gui, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_label_set_text(wifi_scan_header, "Scanning for WiFi APs...");
	lv_obj_set_style_local_text_font(wifi_scan_header, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_24);

	wifi_scan_list = lv_list_create(wifi_scan_gui, NULL);
	lv_obj_set_pos(wifi_scan_list, 0, TOP_HEIGHT);
	lv_obj_set_size(wifi_scan_list, screenWidth, screenHeight-TOP_HEIGHT-BOTTOM_HEIGHT);

	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_size(&style, LV_STATE_DEFAULT, 12);		 /* Width of the scrollbar */
	lv_obj_add_style(wifi_scan_list, LV_LIST_PART_SCROLLBAR, &style);

#if 1
	wifi_scan();
	for (int i = 0; i < ap_count; i++)
	{
		char	buf[128];
		static const char*	auth_mode[WIFI_AUTH_MAX] = {
			/* [WIFI_AUTH_OPEN] = */ "OPEN",
			/* [WIFI_AUTH_WEP] = */ "WEP",
			/* [WIFI_AUTH_WPA_PSK] = */ "WPA_PSK",
			/* [WIFI_AUTH_WPA2_PSK] = */ "WPA2_PSK",
			/* [WIFI_AUTH_WPA2_ENTERPRISE] = */ "WPA2_ENTERPRISE",
			/* [WIFI_AUTH_WPA3_PSK] = */ "WPA3_PSK",
			/* [WIFI_AUTH_WPA2_WPA3_PSK] = */ "WPA3_PSK"
		};
		snprintf(buf, sizeof(buf),
			"%s"					// SSID
			" %s"					// auth_mode
			" (%ddB)"				// RSSI
			" Ch%d"					// Channel
			// " %02X:%02X:%02X:%02X:%02X:%02X"	// MAC
			, ap_info[i].ssid
			, auth_mode[ap_info[i].authmode]
			, ap_info[i].rssi
			, ap_info[i].primary
			// , ap_info[i].bssid[0] , ap_info[i].bssid[1] , ap_info[i].bssid[2]
			// , ap_info[i].bssid[3] , ap_info[i].bssid[4] , ap_info[i].bssid[5]
		);
		lv_obj_t* b = lv_list_add_btn(
				wifi_scan_list,
				ap_info[i].authmode == WIFI_AUTH_OPEN ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE,
				buf // (const char*)ap_info[i].ssid
			);
	}
#endif

	cancel_btn = lv_btn_create(wifi_scan_gui, NULL);
	lv_obj_align(cancel_btn, wifi_scan_gui, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
	lv_obj_set_event_cb(cancel_btn,
		[](lv_obj_t* obj, lv_event_t event){
			if (event == LV_EVENT_CLICKED) show_wifi_credential_ui(false);
		}
	);
	lv_obj_t* label = lv_label_create(cancel_btn, NULL);
	lv_label_set_text(label, "Cancel");
	lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

#if 0
	makePW();
	lv_ex_btnmatrix_1();

	bg_wifiota = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_remove_style(bg_wifiota, NULL, LV_PART_ANY | LV_STATE_ANY);
	// lv_obj_set_style_local_bg_opa(bg_wifiota, LV_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
	// lv_obj_set_style_local_bg_color(bg_wifiota, LV_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_obj_set_pos(bg_wifiota, 0, topHeight);
	lv_obj_set_size(bg_wifiota, LV_HOR_RES, screenHeight - topHeight - bottomHeight);

	lv_obj_move_background(wifi_scan_gui);

	static lv_style_t text_style;
	lv_style_init(&text_style);

	/*Set a background color and a radius*/
	lv_style_set_radius(&text_style, LV_STATE_DEFAULT, 5);
	lv_style_set_bg_opa(&text_style, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_bg_color(&text_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_value_align(&text_style, LV_STATE_DEFAULT, LV_ALIGN_CENTER);
	lv_style_set_text_font(&text_style, LV_STATE_DEFAULT, &lv_font_montserrat_18);

	ota_label = lv_label_create(bg_wifiota, NULL);
	lv_label_set_text(ota_label, "");
	lv_obj_add_style(ota_label, LV_PART_MAIN, &text_style);
	lv_obj_align(ota_label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
	lv_obj_move_background(bg_wifiota);

	ota_label1 = lv_label_create(bg_wifiota, NULL);
	lv_obj_add_style(ota_label1, LV_PART_MAIN, &text_style);
	lv_label_set_text(ota_label1, "");
	lv_obj_align(ota_label1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 30);
	lv_obj_move_background(bg_wifiota);

	wifi_group = lv_group_create();
	lv_indev_set_group(encoder_indev_t, wifi_group);

	lv_group_add_obj(wifi_group, wifi_scan_list);
	lv_group_add_obj(wifi_group, cancel_btn);
#endif
}

static void wifi_scan()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        	ESP_ERROR_CHECK(nvs_flash_erase());
        	ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	uint16_t number = DEFAULT_SCAN_LIST_SIZE;
	memset(ap_info, 0, sizeof(ap_info));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	esp_wifi_scan_start(NULL, true);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_LOGI(LOG_TAG, "Total APs scanned = %u", ap_count);
	for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
		ESP_LOGI(LOG_TAG, "SSID \t\t%s", ap_info[i].ssid);
		ESP_LOGI(LOG_TAG, "RSSI \t\t%d", ap_info[i].rssi);
/*
		print_auth_mode(ap_info[i].authmode);
		if (ap_info[i].authmode != WIFI_AUTH_WEP) {
			print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
		}
*/
		ESP_LOGI(LOG_TAG, "Channel \t\t%d\n", ap_info[i].primary);
	}
}

static void show_wifi_credential_ui(bool show)
{
	if (show)
	{
		lv_obj_move_foreground(wifi_scan_gui);
		/*
		lv_indev_set_group(encoder_indev_t, wifi_group);
		lv_obj_set_hidden(wifi_scan_list, false);
		lv_obj_set_hidden(cancel_btn, false);
		xTaskCreate(wifi_scan_network,"wifi_scan",4096,NULL,1,&hWifiTask);
		*/
	}
}

void create_wifi_credential_button(lv_obj_t* parent)
{
#if 0
	/*
	 * Make a button to open the WiFi gui
	 */
	lvobj*	wifi_button = lv_btn_create(parent, NULL);
	lv_btn_set_checkable(wifi_button, true);
	lv_btn_toggle(wifi_button);
	lv_obj_set_size(wifi_button, 40, 20);
	lv_obj_align(wifi_button, NULL, LV_ALIGN_CENTER, 170, 10);

	lv_obj_t* label = lv_label_create(wifi_button, NULL);
	lv_label_set_text(label, "Wifi");

	lv_obj_add_event_cb(wifi_button, []() {
		show_wifi_credential_ui(true);
	});

	return wifi_button;
#endif
}

static void keyboard_event_cb(lv_obj_t* kb, lv_event_t event)
{
	lv_keyboard_def_event_cb(kb, event);

	if (event == LV_EVENT_APPLY) {
		lv_obj_move_background(keyboard_ui);
/*		const char* password = String(lv_textarea_get_text(ta_password));
		lv_obj_set_hidden(ta_password, true);
		lv_obj_set_hidden(pwd_label, true);
		lv_obj_set_hidden(btnm1, false);
		lv_obj_set_hidden(pwd_label1, false);
*/
	}
	else if (event == LV_EVENT_CANCEL)
	{
		lv_obj_move_background(keyboard_ui);
	}
}

static void makeKeyboard()
{
	keyboard_ui = lv_keyboard_create(lv_scr_act(), NULL);
	lv_obj_set_size(keyboard_ui, LV_HOR_RES, LV_VER_RES / 2);
	// lv_keyboard_set_cursor_manage(keyboard_ui, true);

	// lv_keyboard_set_textarea(keyboard_ui, ta_password);
	lv_obj_set_event_cb(keyboard_ui, keyboard_event_cb);
	lv_obj_move_background(keyboard_ui);
}
