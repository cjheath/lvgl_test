/*
 * WifiScanner: a Thread that owns the WiFi driver and scans for access points.
 */
#include	<string.h>
#include	<stdlib.h>

#include	<esp_wifi.h>
#include	<esp_netif.h>
#include	<esp_event.h>
#include	<nvs_flash.h>

#include	<strval.h>
#include	<variant.h>

#include	"wifi_scanner.h"

static const int	MAX_APS = 40;
static const size_t	SCANNER_STACK_BYTES = 8192;

static const ThreadParams*
scanner_params()
{
	static ThreadParams	params;
	params.stackBytes = SCANNER_STACK_BYTES;
	return &params;
}

WifiScanner::WifiScanner(MessageQueue& a_replies)
: Thread(scanner_params())
, replies(a_replies)
, auto_ms(0)
{
	resume();
}

void
WifiScanner::reply(const VariantArray& message)
{
	replies.push(Variant(message));		// Wrapped, or push() would send each element separately
}

void
WifiScanner::reply_error(const char* what, const char* detail)
{
	VariantArray	message;
	message.push(Variant("error"));
	message.push(Variant(StrVal(what) + StrVal(": ") + StrVal(detail)));
	reply(message);
}

bool
WifiScanner::check(esp_err_t err, const char* what)
{
	if (err == ESP_OK)
		return true;
	reply_error(what, esp_err_to_name(err));
	return false;
}

bool
WifiScanner::start_wifi()
{
	if (!check(nvs_flash_init(), "nvs_flash_init"))
		return false;
	if (!check(esp_netif_init(), "esp_netif_init"))
		return false;

	esp_err_t	err = esp_event_loop_create_default();
	if (err != ESP_ERR_INVALID_STATE && !check(err, "esp_event_loop_create_default"))
		return false;		// ESP_ERR_INVALID_STATE: the loop already exists, which is fine

	if (!esp_netif_create_default_wifi_sta())
	{
		reply_error("esp_netif_create_default_wifi_sta", "failed");
		return false;
	}

	wifi_init_config_t	config = WIFI_INIT_CONFIG_DEFAULT();
	return check(esp_wifi_init(&config), "esp_wifi_init")
		&& check(esp_wifi_set_mode(WIFI_MODE_STA), "esp_wifi_set_mode")
		&& check(esp_wifi_start(), "esp_wifi_start");
}

static const char*
auth_name(wifi_auth_mode_t auth)
{
	switch (auth)
	{
	case WIFI_AUTH_OPEN:		return "open";
	case WIFI_AUTH_WEP:		return "WEP";
	case WIFI_AUTH_WPA_PSK:		return "WPA";
	case WIFI_AUTH_WPA2_PSK:	return "WPA2";
	case WIFI_AUTH_WPA_WPA2_PSK:	return "WPA/WPA2";
	case WIFI_AUTH_WPA3_PSK:	return "WPA3";
	case WIFI_AUTH_WPA2_WPA3_PSK:	return "WPA2/WPA3";
	case WIFI_AUTH_OWE:		return "OWE";
	default:			return "other";
	}
}

// An SSID is up to 32 bytes of anything, usually UTF-8
static StrVal
ssid_string(const uint8_t* ssid)
{
	size_t		length = strnlen((const char*)ssid, 32);
	for (size_t i = 0; i < length; )
	{
		uint8_t		c = ssid[i];
		int		extra = c < 0x80 ? 0 : (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
		bool		bad = extra < 0 || i+extra >= length;
		for (int j = 1; !bad && j <= extra; j++)
			bad = (ssid[i+j] & 0xC0) != 0x80;
		if (bad)
			return StrVal((const char*)ssid, (StrValIndex)length, 0, ArrayCopy, StrRawBinary);
		i += extra+1;
	}
	return StrVal((const char*)ssid, (StrValIndex)length);
}

static StrVal
bssid_string(const uint8_t* bssid)
{
	static const char	digits[] = "0123456789abcdef";
	char			text[18];
	for (int i = 0; i < 6; i++)
	{
		text[i*3] = digits[bssid[i] >> 4];
		text[i*3+1] = digits[bssid[i] & 15];
		text[i*3+2] = i < 5 ? ':' : '\0';
	}
	return StrVal(text);
}

void
WifiScanner::scan()
{
	wifi_scan_config_t	config;
	memset(&config, 0, sizeof config);	// All channels, active scan, hidden APs not shown
	if (!check(esp_wifi_scan_start(&config, true), "esp_wifi_scan_start"))
		return;

	uint16_t		found = 0;
	if (!check(esp_wifi_scan_get_ap_num(&found), "esp_wifi_scan_get_ap_num"))
		return;
	uint16_t		count = found < MAX_APS ? found : MAX_APS;

	wifi_ap_record_t*	records = (wifi_ap_record_t*)calloc(count ? count : 1, sizeof *records);
	if (!records)
	{
		esp_wifi_clear_ap_list();
		reply_error("scan", "out of memory");
		return;
	}
	if (!check(esp_wifi_scan_get_ap_records(&count, records), "esp_wifi_scan_get_ap_records"))
	{
		free(records);
		return;
	}

	// The driver returns them strongest-first already, but doesn't promise to
	for (int i = 1; i < count; i++)
	{
		wifi_ap_record_t	r = records[i];
		int			j = i;
		for (; j > 0 && records[j-1].rssi < r.rssi; j--)
			records[j] = records[j-1];
		records[j] = r;
	}

	VariantArray		aps;
	for (int i = 0; i < count; i++)
	{
		VariantArray	ap;
		ap.push(Variant(ssid_string(records[i].ssid)));
		ap.push(Variant((int)records[i].rssi));
		ap.push(Variant((int)records[i].primary));
		ap.push(Variant(auth_name(records[i].authmode)));
		ap.push(Variant(bssid_string(records[i].bssid)));
		aps.push(Variant(ap));
	}
	free(records);

	VariantArray		message;
	message.push(Variant("scan"));
	message.push(Variant(aps));
	reply(message);
}

int
WifiScanner::run()
{
	if (!start_wifi())
		return 1;

	VariantArray		ready;
	ready.push(Variant("ready"));
	reply(ready);

	for (;;)
	{
		Variant		request = auto_ms > 0 ? requests.pop(Milliseconds(auto_ms)) : requests.pop();
		if (request.is_null())
		{		// The interval passed with no request
			scan();
			continue;
		}

		if (request.type() != Variant::VarArray)
		{
			reply_error("request", "not an array");
			continue;
		}
		VariantArray	args = request.as_variant_array();
		if (args.length() == 0 || args[0].type() != Variant::String)
		{
			reply_error("request", "no command name");
			continue;
		}

		StrVal		command = args[0].as_strval();
		if (command == "scan")
			scan();
		else if (command == "auto")
		{
			if (args.length() != 2 || args[1].type() != Variant::Integer || args[1].as_int() < 0)
			{
				reply_error("auto", "needs a count of milliseconds, or 0");
				continue;
			}
			auto_ms = args[1].as_int();
		}
		else if (command == "quit")
		{
			bool	stopped = check(esp_wifi_stop(), "esp_wifi_stop")
				&& check(esp_wifi_deinit(), "esp_wifi_deinit");
			VariantArray	bye;
			bye.push(Variant("quit"));
			reply(bye);
			return stopped ? 0 : 1;
		}
		else
			reply_error("request", "unknown command");
	}
}
