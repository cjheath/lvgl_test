## Test program for LVGL on ESP-IDF and ILI9341

Just a stub at present, to test the component build process.
Rather than using LVGL directly, I'm building the `ui_task` component as a wrapper for some details.
Next up is to implement WiFi AP scanning, selection and password entry.

The LVGL app is incomplete: it shows a placeholder screen and runs touch calibration.
The WiFi scanning so far is in `components/wifi_scanner`, which you can try without an LCD
using the console app below.

### Setup:
- Install ESP-IDF (and VSCode, if you wish)
- Download the code using `git clone --recurse-submodules` to get all submodules
- Configure ESP-IDF environment variables (I use `. ~/.espressif/esp-idf/export.sh`)
- Use `idf.py menuconfig` or the VSCode ESP-IDF SDK Configuration Editor to select your LCD hardware
- Build using one of the commands below, or VSCode

### Build the LVGL app (ESP32)
    idf.py build
    idf.py -p <port> flash monitor

### Build the console app (ESP32-S3, no LCD needed)
The console app scans for WiFi networks over the USB serial port. It uses `sdkconfig.console`
and its own build directory, so it doesn't disturb the LVGL build:

    idf.py -B build_console -DSDKCONFIG=build_console/sdkconfig \
        -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.console" \
        -DIDF_TARGET=esp32s3 build
    idf.py -B build_console -DSDKCONFIG=build_console/sdkconfig \
        -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.console" \
        -DIDF_TARGET=esp32s3 -p <port> flash
    idf.py -B build_console -p <port> monitor

- Use the board's native USB port (it shows up as `/dev/cu.usbmodem...`), not a UART bridge:
  the console is on USB-Serial-JTAG, and flashing through a bridge failed here.
- Configuring for the ESP32-S3 rewrites `dependencies.lock`; run `git checkout dependencies.lock` afterwards.
- At the `wifi>` prompt, type `scan` and press Enter. Other commands: `auto <secs>`, `info`, `tasks`, `quit`, `help`.

### More information on LVGL setup
- See https://docs.lvgl.io/master/get-started/espressif.html

