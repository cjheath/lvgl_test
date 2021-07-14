/*
 * LVGL GUI Display
 */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lvgl.h"

namespace LVGL {

class Display
{
public:
	// Initialise a display
	Display();
	~Display();

	// Call this lambda under control of the display semaphore
	void			synchronised(void (*)());

protected:
	lv_disp_t*		display;
	lv_disp_drv_t*		display_driver;
	SemaphoreHandle_t	gui_semaphore;
	lv_color_t*		buf1;
	lv_color_t*		buf2;
	lv_disp_draw_buf_t*	draw_buf;
	lv_indev_drv_t*		input_driver;
	esp_timer_handle_t	periodic_timer;

	void			init_driver();
	void			init_pointer_device();
	void			start_gui_timer();
	void			run();
};

}
