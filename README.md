## Test program for LVGL on ESP-IDF and ILI9341

Just a stub at present, to test the component build process.
Rather than using LVGL directly, I'm building the `ui_task` component as a wrapper for some details.
Next up is to implement WiFi AP scanning, selection and password entry.

The LVGL app is incomplete: it shows a placeholder screen and runs touch calibration.
The WiFi scanning so far is in `components/wifi_scanner`. You can try it without an LCD using
the console test driver in strpp, `test/freertos_console`.

### Setup:
- Install ESP-IDF (and VSCode, if you wish)
- Download the code using `git clone --recurse-submodules` to get all submodules
- Configure the ESP-IDF environment: `. ~/.espressif/tools/activate_idf_v6.1.sh` (the script the ESP-IDF installer writes for v6.1)
- Use `idf.py menuconfig` or the VSCode ESP-IDF SDK Configuration Editor to select your LCD hardware
- Build using the commands below, or VSCode

### Build the LVGL app (ESP32)
    idf.py build
    idf.py -p <port> flash monitor

### More information on LVGL setup
- See https://docs.lvgl.io/master/get-started/espressif.html

