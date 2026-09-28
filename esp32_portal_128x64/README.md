# led_matrix_clock - MatrixPortal ESP32-S3, 128x64

Port of `small_esp32_64x32` to the Adafruit MatrixPortal ESP32-S3 with a 128x64 HUB75 panel (Protomatter).

- `clock_128` - PlatformIO firmware project
- `main_ui` - web UI, served by the ESP32 from LittleFS
- `py_server` - local server for testing the UI without flashing. Go to `main_ui` and run `python ..\py_server\main.py 80`
- `adafruit_gfx_emu` - Adafruit_GFX + fonts compiled to wasm so the UI can preview text rendering.
  `build\build_rel.bat` builds it and copies `asm_gfx.js`/`asm_gfx.wasm` into `main_ui`

Instructions:
- WiFi credentials are in `clock_128/src/wifi_secrets.h`
- flash the firmware: `pio run -t upload` in `clock_128`
- upload the web UI to LittleFS: `pio run -t uploadfs` in `clock_128` (`data_dir` in platformio.ini points to `../main_ui`)
- open `http://myesp32clock.local/`

Buttons: UP click - next section, DOWN click - start/stop,
UP long-press - change +1 (timer +1 min), DOWN long-press - change -1 (timer -1 min, stop watch reset)
