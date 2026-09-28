rem --profiling -s DEMANGLE_SUPPORT=1
rem TBD BUG this doesn't work without -g3
rem -O3 

rem "call" since em++ is a .bat, without it this script ends after it
call em++ --bind -s DEMANGLE_SUPPORT=0 -s ALLOW_MEMORY_GROWTH=0 -s WASM=1 -Oz --memory-init-file 0  -Wno-switch -I. -I.. ../Adafruit_GFX.cpp ../arduino/Print.cpp ../arduino/WString.cpp ../js_main.cpp  -o asm_gfx.html

rem the web UI needs real files (not symlinks) so they can be uploaded to the ESP32 file-system
copy /Y asm_gfx.js ..\..\main_ui\
copy /Y asm_gfx.wasm ..\..\main_ui\
