rem --profiling -s DEMANGLE_SUPPORT=1
rem TBD BUG this doesn't work without -g3
rem -O3 

em++ --bind -s DEMANGLE_SUPPORT=0 -s ALLOW_MEMORY_GROWTH=0 -s WASM=1 -Oz --memory-init-file 0  -Wno-switch -I. -I.. ../Adafruit_GFX.cpp ../arduino/Print.cpp ../arduino/WString.cpp ../js_main.cpp  -o asm_gfx.html 

