#include <iostream>
#include <sstream>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "Adafruit_GFX.h"
#include "../clock/my_fonts/fonts_index.h"

class JsDisplay : public Adafruit_GFX
{
public:
    JsDisplay(emscripten::val canvas, int w, int h) :Adafruit_GFX(w, h), m_canvas(canvas)
    {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        //std::cout << "draw " << x << "," << y << " : " << color << std::endl;
        auto [r, g, b] = color888(color);
        m_canvas.call<void>("setPixel", x, y, r, g, b);
    }

    void print_str_at(int x, int y, const std::string& s)
    {
        setCursor(x, y);
        print(String(s.c_str()));
    }

    // https://stackoverflow.com/questions/2442576/how-does-one-convert-16-bit-rgb565-to-24-bit-rgb888
    uint16_t color565(uint8_t R8, uint8_t G8, uint8_t B8) 
    {
        int R5 = ( R8 * 249 + 1014 ) >> 11;
        int G6 = ( G8 * 253 +  505 ) >> 10;
        int B5 = ( B8 * 249 + 1014 ) >> 11;
        return ( (B5 & 0x1F) | ((G6 & 0x3F) << 5) | ((R5 & 0x1F) << 11) );
    }

    std::tuple<uint8_t, uint8_t, uint8_t> color888(uint32_t v)
    {
        uint16_t R5 = (v >> 11) & 0x1F;
        uint16_t G6 = (v >> 5) & 0x3F;
        uint16_t B5 = v & 0x1F;
        uint8_t R8 = ( R5 * 527 + 23 ) >> 6;
        uint8_t G8 = ( G6 * 259 + 33 ) >> 6;
        uint8_t B8 = ( B5 * 527 + 23 ) >> 6;
        return std::make_tuple(R8, G8, B8);
    }

    void set_text_color(int r, int g, int b)
    {
        setTextColor(color565(r, g, b));
    }

    void set_font(int index)
    {
        if (index == -1) {
            setFont(nullptr);
            return;
        }
        if (index < 0 || index >= (sizeof(all_fonts) / sizeof(FontDef)))
            return;
        setFont(all_fonts[index].fontPtr);
    }

    void clear()
    {
        fillScreen(0);
    }

private:
    emscripten::val m_canvas;
};


void gfx_init_display(emscripten::val canvas, int w, int h)
{
    std::shared_ptr<JsDisplay> disp(new JsDisplay(canvas, w, h));
    canvas.set("gfx", emscripten::val(disp));
    disp->setTextWrap(false);

    //disp->print("Hello");
}

std::vector<std::string> gfx_get_fonts()
{
    std::vector<std::string> lst;
    for(int i = 0; i < (sizeof(all_fonts) / sizeof(FontDef)); ++i)
    {
        const auto& f = all_fonts[i];
        std::stringstream ss;
        ss << f.familyName << " " << f.fontSize;
        lst.push_back(ss.str());
    }
    return lst;
}

// TODO remove most zeros from bitmaps
// TODO support wider chars

EMSCRIPTEN_BINDINGS(my_module)
{
    emscripten::function("gfx_init_display", &gfx_init_display);
    emscripten::register_vector<std::string>("StringList");
    emscripten::function("gfx_get_fonts", &gfx_get_fonts);

    emscripten::class_<JsDisplay>("JsDisplay")
        .smart_ptr<std::shared_ptr<JsDisplay>>("JsDisplay")
        .function("print_str_at", &JsDisplay::print_str_at)
        .function("set_text_color", &JsDisplay::set_text_color)
        .function("set_font", &JsDisplay::set_font)
        .function("clear", &JsDisplay::clear)
        ;
    
}