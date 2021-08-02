## Test program for LVGL on ESP-IDF and ILI9341

Just a stub at present, to test the component build process.
Rather than using LVGL directly, I'm building the `ui_task` component as a wrapper for some details.
Next up is to implement WiFi AP scanning, selection and password entry.

### Setup:
- Install ESP-IDF (and VSCode, if you wish)
- Download the code using `git clone --recurse-submodules` to get all submodules
- Configure ESP-IDF environment variables (I use `. ~/.espressif/esp-idf/export.sh`)
- Use `idf.py menuconfig` or the VSCode ESP-IDF SDK Configuration Editor to select your LCD hardware
- Apply this patch: `patch -p1 lvgl_esp32_drivers.patch`
- Build using `idf.py build` or VSCode

### More information on LVGL setup
- See https://docs.lvgl.io/master/get-started/espressif.html

