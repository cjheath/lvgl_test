/*
 * LVGL GUI Display
 */
#if !defined(UI_TASK_H)
#define	UI_TASK_H

#include "lvgl.h"

namespace LVGL {

class Display
{
public:
	// Initialise a display
	Display();
	~Display();

	// Call this lambda under the LVGL lock, protecting concurrent access
	void			synchronised(void (*)(Display*));

	lv_disp_t*		display;
	lv_indev_t*		touch_indev;

private:
	void			init_panel();
	void			init_touch();
	void			init_calibration();
};

}
#endif // UI_TASK_H
