/*
 * LVGL GUI Display
 */
#include "sdkconfig.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#include "esp_nvs_tc.h"
#include "lv_tc.h"
#include "lv_tc_screen.h"

#include "ui_task.h"

using namespace LVGL;

static const char*		TAG = "ui_task";

#if defined(CONFIG_UI_DISP_SPI_HOST_SPI2)
static const spi_host_device_t	DISP_SPI_HOST = SPI2_HOST;
#else
static const spi_host_device_t	DISP_SPI_HOST = SPI3_HOST;
#endif

static esp_lcd_touch_handle_t	s_touch_handle;
static esp_lcd_panel_io_handle_t s_disp_io_handle;
static esp_lcd_panel_handle_t	s_panel_handle;

static void show_placeholder_screen();

static void touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
	esp_lcd_touch_read_data(s_touch_handle);

	uint16_t	x, y;
	uint8_t		count = 0;
	bool		pressed = esp_lcd_touch_get_coordinates(s_touch_handle, &x, &y, NULL, &count, 1);

	if (pressed && count > 0)
	{
		data->point.x = x;
		data->point.y = y;
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else
		data->state = LV_INDEV_STATE_RELEASED;
}

static void calibration_finished_cb(lv_event_t* e)
{
	show_placeholder_screen();
}

static void placeholder_touch_cb(lv_event_t* e)
{
	lv_obj_t*	label = (lv_obj_t*)lv_event_get_user_data(e);
	lv_point_t	p;

	lv_indev_get_point(lv_indev_get_act(), &p);
	lv_label_set_text_fmt(label, "Calibrated OK\ntouch at %d, %d", (int)p.x, (int)p.y);
}

static void show_placeholder_screen()
{
	lv_obj_t*	screen = lv_obj_create(NULL);
	lv_obj_t*	label = lv_label_create(screen);

	lv_label_set_text(label, "Calibrated OK");
	lv_obj_center(label);
	lv_obj_add_event_cb(screen, placeholder_touch_cb, LV_EVENT_PRESSING, label);
	lv_disp_load_scr(screen);
}

// Initialise a display
Display::Display()
: display(0)
, touch_indev(0)
{
	init_panel();
	init_touch();
	init_calibration();
}

Display::~Display()
{
	// REVISIT: GUI task never exits so this can do nothing useful
}

void
Display::synchronised(void (*lambda)(Display*))
{
	if (lvgl_port_lock(0))
	{
		lambda(this);
		lvgl_port_unlock();
	}
}

void Display::init_panel()
{
	spi_bus_config_t	buscfg = {};
	buscfg.mosi_io_num = CONFIG_UI_DISP_SPI_MOSI_GPIO;
	buscfg.miso_io_num = CONFIG_UI_DISP_SPI_MISO_GPIO;
	buscfg.sclk_io_num = CONFIG_UI_DISP_SPI_CLK_GPIO;
	buscfg.quadwp_io_num = -1;
	buscfg.quadhd_io_num = -1;
	buscfg.max_transfer_sz = CONFIG_UI_DISP_HOR_RES * 40 * sizeof(uint16_t);
	ESP_ERROR_CHECK(spi_bus_initialize(DISP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

	esp_lcd_panel_io_spi_config_t	io_config = {};
	io_config.cs_gpio_num = (gpio_num_t)CONFIG_UI_DISP_CS_GPIO;
	io_config.dc_gpio_num = (gpio_num_t)CONFIG_UI_DISP_DC_GPIO;
	io_config.spi_mode = 0;
	io_config.pclk_hz = 40 * 1000 * 1000;
	io_config.trans_queue_depth = 10;
	io_config.lcd_cmd_bits = 8;
	io_config.lcd_param_bits = 8;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &io_config, &s_disp_io_handle));

	esp_lcd_panel_dev_config_t	panel_config = {};
	panel_config.reset_gpio_num = (gpio_num_t)CONFIG_UI_DISP_RST_GPIO;
	panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
	panel_config.bits_per_pixel = 16;
	ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(s_disp_io_handle, &panel_config, &s_panel_handle));

	ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

	gpio_set_direction((gpio_num_t)CONFIG_UI_DISP_BCKL_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)CONFIG_UI_DISP_BCKL_GPIO, 1);

	const lvgl_port_cfg_t	lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
	ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

	lvgl_port_display_cfg_t	disp_cfg = {};
	disp_cfg.io_handle = s_disp_io_handle;
	disp_cfg.panel_handle = s_panel_handle;
	disp_cfg.buffer_size = CONFIG_UI_DISP_HOR_RES * 40;
	disp_cfg.double_buffer = true;
	disp_cfg.hres = CONFIG_UI_DISP_HOR_RES;
	disp_cfg.vres = CONFIG_UI_DISP_VER_RES;
	disp_cfg.flags.buff_dma = true;
	disp_cfg.flags.swap_bytes = true;
	display = lvgl_port_add_disp(&disp_cfg);
	if (display == NULL)
		ESP_LOGE(TAG, "lvgl_port_add_disp failed");
}

void Display::init_touch()
{
	// ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG leaves some fields unlisted; they're still
	// zero-initialized, but -Werror=missing-field-initializers doesn't know that.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	esp_lcd_panel_io_spi_config_t	tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(CONFIG_UI_TOUCH_CS_GPIO);
#pragma GCC diagnostic pop
	esp_lcd_panel_io_handle_t	tp_io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &tp_io_config, &tp_io_handle));

	esp_lcd_touch_config_t	tp_cfg = {};
	tp_cfg.x_max = CONFIG_UI_DISP_HOR_RES;
	tp_cfg.y_max = CONFIG_UI_DISP_VER_RES;
	tp_cfg.rst_gpio_num = GPIO_NUM_NC;
	tp_cfg.int_gpio_num = (gpio_num_t)CONFIG_UI_TOUCH_IRQ_GPIO;
	ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &s_touch_handle));

	touch_indev = lv_indev_create();
	lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(touch_indev, touch_read_cb);

	lv_tc_indev_init(touch_indev);
	lv_tc_register_coeff_save_cb(esp_nvs_tc_coeff_save_cb);
}

void Display::init_calibration()
{
	lv_obj_t*	tc_screen = lv_tc_screen_create();

	lv_obj_add_event_cb(tc_screen, calibration_finished_cb, LV_EVENT_READY, NULL);

	if (esp_nvs_tc_coeff_init())
		show_placeholder_screen();
	else
	{
		lv_disp_load_scr(tc_screen);
		lv_tc_screen_start(tc_screen);
	}
}
