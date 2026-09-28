# PixelDust demo on MatrixPortal ESP32-S3 (128x64): issues and fixes

Setup: Adafruit MatrixPortal ESP32-S3, 128x64 HUB75 panel, `src/pixeldust.cpp`,
Adafruit_Protomatter 1.7.1 and Adafruit_PixelDust vendored under `lib/`,
arduino-esp32 core 2.0.17 (IDF 4.4).

Three separate problems were found and fixed.

---

## 1. Frozen black block and missing colors

**Symptom:** the bottom 8 rows of the right half (x = 64..127) never moved,
stayed black, and the other grains flowed around them. Only some of the colors
showed up.

**Cause:** `N_GRAINS` was still set for the original 64-pixel-wide example:

```cpp
#define N_GRAINS (BOX_HEIGHT*N_COLORS*8)   // = 512
```

At `WIDTH 128` each color box is 16 pixels wide, so `setup()` places
8 x 16 x 8 = 1024 grains. The simulation was created with only 512:

- Grains 512..1023 (colors 4..7, the right half) were marked as occupied in
  PixelDust's collision bitmap but never simulated or drawn. They acted as
  invisible, fixed obstacles.
- `setPosition()` does no bounds check, so writing `grain[512..1023]` wrote
  about 6 KB past the end of the heap-allocated grain array.

**Fix** (`src/pixeldust.cpp`):

```cpp
#define N_GRAINS (BOX_HEIGHT*N_COLORS*(WIDTH/N_COLORS))
```

---

## 2. Slow animation (15 FPS)

**Symptom:** the frame rate was exactly 15 FPS. Timing each part of the loop
showed `sand.iterate()` taking about 56 ms per frame.

**Cause:** `iterate()` calls `random()` twice per grain per frame (about 2048
calls). On arduino-esp32, `random()` uses the hardware RNG (`esp_random()`) by
default. With WiFi/BT off, `esp_random()` deliberately waits between reads to
gather enough randomness, so each call costs tens of microseconds.

**Fix** (`src/pixeldust.cpp`, in `setup()`):

```cpp
randomSeed(esp_random());
```

Calling `randomSeed()` switches `random()` to the software PRNG (`rand()`). The
simulation then takes about 3 ms and the sketch runs at its 45 FPS cap.

---

## 3. Flickering runs of wrong pixels

**Symptom:** a few times a second, short runs (5-10 pixels) of wrongly colored
(mostly red/blue) pixels flickered in rows that contained grains, near the
panel edge. The flicker happened more often at 45 FPS than at 15.

### Ruled out

| Test | Result |
|---|---|
| Per-frame check for grains jumping >1 px or two grains on one pixel | 0 always, so the simulation's output is correct |
| Pixel clock lowered (`LCD_CLK_PRESCALE` 9 -> 10) | no change, so not cable signal quality |
| Timing slack doubled (`_PM_minMinPeriod` 200 -> 400 ticks) | no change (the stalls turned out to be much longer) |
| Simulation kept running while the display showed a frozen, scattered image | flicker still present, so it's not tied to the image changing |

### Cause

Protomatter's ESP32-S3 code (`lib/adafruit_protomatter/src/arch/esp32-s3.h`)
sends each bitplane of a row to the panel with an LCD_CAM DMA transfer. The row
interrupt:

1. latches the previously sent data and starts the timer for the next row,
2. then starts the next DMA transfer (`blast_byte()`).

No interrupt fires when the transfer finishes. The library only estimates its
length and allows about 200 timer ticks (5 us at 40 MHz) of slack for DMA
setup. The interrupt handler itself is in IRAM, but it calls functions that run
from flash:

- the arduino-esp32 2.0.x timer wrappers (`timerAlarmWrite`, `timerStart`,
  `timerRead`, ...), and
- `gdma_start()` (`CONFIG_GDMA_CTRL_FUNC_IN_IRAM` is not set in the Arduino
  sdkconfig).

While the sand simulation runs it evicts that code from the instruction cache,
so the DMA start sometimes stalls. Temporary instrumentation measured DMA setup
times of **700-990 ticks (18-25 us)** and about **30 early latches per
second**. An early latch means the row is latched before all its pixels have
been shifted in, so the pixels at the input end of the chain show stale data.

### Fix

Rather than guessing a larger slack, the row interrupt now waits until the
transfer has actually finished before latching:

- `blast_byte()` clears the LCD_CAM `TRANS_DONE` raw interrupt flag right
  before starting each transfer and sets `_PM_dmaPending`.
- New `_PM_waitDmaDone()` (IRAM) spins until `TRANS_DONE` is set, with an upper
  bound so a stuck flag can't hang the ISR.
- `core.c` `_PM_row_handler()` calls a new optional hook,
  `_PM_rowHandlerStartHook(core)`, first thing. `esp32-s3.h` defines it as
  `_PM_waitDmaDone()`; other architectures don't define it and are unaffected.

The cost is that on the ~30 occasions per second when DMA is late, the bitplane
currently shown stays lit a few microseconds longer. This isn't visible.

Changed files:

- `lib/adafruit_protomatter/src/arch/esp32-s3.h`: `_PM_dmaPending`,
  `_PM_waitDmaDone()`, `_PM_rowHandlerStartHook`, and the flag clear in
  `blast_byte()`.
- `lib/adafruit_protomatter/src/core.c`: the hook call at the start of
  `_PM_row_handler()`.

**Note:** this is a local change to the vendored library. It will be lost if
Protomatter is updated or reinstalled from the library manager. Upstream
`esp32-s3.h` (checked 2026-09) has the same timing logic without a DMA-done
wait, so the fix is worth reporting upstream:
https://github.com/adafruit/Adafruit_Protomatter/issues

---

## Other notes

- `iterate()` takes `int16_t` arguments. The sketch passes acceleration x 1000
  (m/s^2), which overflows above about 3.3 g. That can happen when shaking the
  board with the +/-4 g range. Scale down if grains jump oddly when shaken.
- The sketch prints `FPS: N` to serial once per second.
