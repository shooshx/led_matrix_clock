rem %EMSCRIPTEN%\emcc --clear-cache
rem --tracing
rem -D_LIBCPP_DEBUG=0  --bind


rem run C:\lib\emscripten\emsdk\emsdk_env.bat before


em++ -g3 -O0 --bind -s ASSERTIONS=2 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -D_DEBUG -D_LIBCPP_ENABLE_DEBUG_MODE=0 --memory-init-file 0 -Wno-switch -I. -I.. ../Adafruit_GFX.cpp ../arduino/Print.cpp ../arduino/WString.cpp ../js_main.cpp  -o asm_gfx.html 


rem Pointer_stringify