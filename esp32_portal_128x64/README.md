# led_matrix_clock - MatrixPortal ESP32-S3, 128x64

Port of `small_esp32_64x32` to the Adafruit MatrixPortal ESP32-S3 with a 128x64 HUB75 panel (Protomatter).

- `clock_128` - PlatformIO firmware project
- `main_ui` - web UI, served by the ESP32 from LittleFS
- `sounds` - sound sources; `convert_sounds.py` makes the mono mp3s that go to the panel's `/sounds`
- `py_server` - local server for testing the UI without flashing. Go to `main_ui` and run `python ..\py_server\main.py 80`
- `adafruit_gfx_emu` - Adafruit_GFX + fonts compiled to wasm so the UI can preview text rendering.
  `build\build_rel.bat` builds it and copies `asm_gfx.js`/`asm_gfx.wasm` into `main_ui`
- `doc` - notes, `future_features.md`

Instructions:
- WiFi credentials are in `clock_128/src/wifi_secrets.h`
- flash the firmware: `pio run -t upload` in `clock_128`
- upload the web UI and the sounds to LittleFS: `pio run -t uploadfs` in `clock_128` (`gzip_web.py` builds `.pio/web_data`)
- open `http://myesp32clock.local/`
- if the esp starts and shows the drive, press the boot button and reset
- run the UI locally: `cd main_ui`, `python ../py_server/main.py`

## Features

- Clock with day periods, each showing a clock face; faces can be Normal, Off (dark) or Random
- Day types (e.g. Weekday, Off-day), each with its own periods; each day of the week picks one
- Face editor in the web UI: texts, fonts, colors, outlines, landscape and portrait layouts, live preview
- Background effects: Border, Slide, Twinkle, Fireflies, Aurora, Rain, Snow, Fire, Ripples, Radar, Plasma, Warp, Lava, Tunnel, Cube
- Random faces: everything by chance, a new one every set interval
- Face sounds: random picks from chosen mp3s, every N, for a total time; the period start sets the face's volume
- A period start switches to the clock (the face's mode decides lit or dark), except during a game
- "Show on panel" previews a period on the panel, sounds included
- Timer, Stopwatch
- Draw: paint from the web UI or the remote, type text with a keyboard
- Image: show an image sent from the web UI
- Games: Dino, Tetris, Life, Dust, Effects (any effect full screen)
- Audio: MAX98357A I2S amp (BCLK A4, LRC A3, DIN A2); test beep, mp3 playback, one saved volume
- BLE remotes/keyboards (HID), several at once, pairing from the admin view
- Auto rotation from the accelerometer, brightness, gamma, color depth settings
- Admin view: network, time zone, brightness, volume, sounds, remote sensitivity, pairing, IMU

## Keys

### Board buttons
- At boot, hold UP or DOWN: admin view
- UP click: next mode; UP long-press: +1 (timer +1 min)
- DOWN click: start/stop; DOWN long-press: -1 (timer -1 min, stopwatch reset)

### Remote / keyboard, everywhere
- Power: display off/on; Up/Down also turn it on
- Home held + Power (display off): admin view
- Up/Down: previous/next mode (Clock, Stopwatch, Timer, Draw, Games), except inside a game or while typing
- Volume +/-: volume (with an on-screen bar), except in Timer, Draw, Life, Tetris and Effects
- Esc: start typing (goes to Draw); in a game it pauses like Back

### Timer
- OK: start/stop; Back: stop and reset to the set time
- Left/Right: -/+ 1 minute; Volume +/-: +/- 10 minutes

### Stopwatch
- OK: start/stop; Back: stop and reset to 0

### Draw
- Move the remote: cursor; hold OK: draw
- Left/Right or PgUp/PgDn: color; Volume +/-: pen size
- Back: asks to clear (OK clears, Back cancels)
- Esc: start/stop typing; while typing: arrows move the cursor, PgUp/PgDn color, Caps Lock shown

### Games menu
- Left/Right: pick a game; OK: play
- In a game, Back: pause (OK continues, Back again to the menu)

### Dino
- Up or OK: jump; Down: duck

### Tetris (portrait)
- Left/Right: move; Up or Volume+: rotate; Volume-: rotate back; Down: faster; OK: drop

### Life
- Home: pause/resume; Volume +/-: speed
- Move the remote + hold OK: draw cells; Left/Right: color; Up/Down: pen size
- Pause menu: Home random, Menu clear

### Dust
- Tilt the panel: sand falls; Home: pause/resume
- Move the remote + hold OK: pour sand; Left/Right: color (last one erases); Up/Down: pen size
- Pause menu: Home restart, Menu clear

### Effects
- Up/Down: effect; Left/Right: color (12 hues); Volume +/-: size; OK: speed; Home: Slide angle +15°

### Admin view
- Up/Down: select a line; Left/Right: change it; Back: to the clock
- Sound line: Left/Right pick, OK plays; "Pair new device": OK starts pairing
- Pairing: Up/Down select a device, OK pairs, Back/Esc/Left stops
