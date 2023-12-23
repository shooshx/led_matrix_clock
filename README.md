# led_matrix_clock
An Arduino based LED matrix clock

instructions:
- build font header files by running python .bdf2adafruit\bdf2adafruit.py
- build wasm by going to adafruit_gfx_emu\build and running build_rel.py
- upload pages to Arduino by using the SPIFFS uploader


TODO:
- animations for timer, stop watch (cursor?)
- timer alarm changes active_section
- after some time with of inactivity go back to clock
- font editor
- GPIO buttons
- color offset in display?
- animations
    - game of life
    - plasma
    - AuroraDemo- https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA/tree/master/examples/AuroraDemo

sprites:
   https://www.spriters-resource.com/nes/A.html


TODO: 
wasm mime type  in WebResponses.cpp


read about priorities in RTOS


just reimpl the file handler to lock