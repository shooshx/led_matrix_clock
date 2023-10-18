
#include <iostream>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "Adafruit_GFX.h"

class JsDisplay : public Adafruit_GFX
{
public:
    JsDisplay(emscripten::val canvas, int w, int h) :Adafruit_GFX(w, h), m_canvas(canvas)
    {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        //std::cout << "draw " << x << "," << y << " : " << color << std::endl;
        m_canvas.call<void>("setPixel", x, y, 255, 0, 0);
    }

    void print_str_at(int x, int y, const std::string& s)
    {
        setCursor(x, y);
        print(String(s.c_str()));
    }

private:
    emscripten::val m_canvas;
};


void gfx_init_display(emscripten::val canvas, int w, int h)
{
    std::shared_ptr<JsDisplay> disp(new JsDisplay(canvas, w, h));
    canvas.set("gfx", emscripten::val(disp));

    disp->print("Hello");
}


EMSCRIPTEN_BINDINGS(my_module)
{
    emscripten::function("gfx_init_display", &gfx_init_display);

    emscripten::class_<JsDisplay>("JsDisplay")
        .smart_ptr<std::shared_ptr<JsDisplay>>("JsDisplay")
        .function("print_str_at", &JsDisplay::print_str_at)
        ;
}