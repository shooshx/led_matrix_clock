#include <iostream>
#include <sstream>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "Adafruit_GFX.h"
#include "../clock/my_fonts/fonts_index.h"
#include "../clock/drawer.h"

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

std::vector<uint8_t> color888v(uint32_t v)
{
    auto [r, g, b] = color888(v);
    return std::vector<uint8_t>{r, g, b};
}


class JsDraw : public IDraw //, public std::enable_shared_from_this<JsDraw> 
{
public:
    JsDraw(emscripten::val jcanvas) : m_jcanvas(jcanvas)
    {
        //jcanvas.set("drawer", emscripten::val(shared_from_this()));
    }
    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override {
        m_jcanvas.call<void>("setPixel", x, y, r, g, b);
    }
    void flush() override {
    }
    void clear() override {
    }
private:
    emscripten::val m_jcanvas;
};


class JsDisplay : public Adafruit_GFX
{
public:
    JsDisplay(int w, int h) :Adafruit_GFX(w, h)
    {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        //std::cout << "draw " << x << "," << y << " : " << color << std::endl;
        auto [r, g, b] = color888(color);
        m_drawer->setPixel(x, y, r, g, b);
    }

    void set_drawer(IDraw* drawer) {
        m_drawer = drawer;
    }

    void print_str_at(int x, int y, const std::string& s, int align=0)
    {
        if (align == 0)
            setCursor(x, y);
        else {
            int16_t x1 = 0, y1 = 0, x_after = 0;
            uint16_t w = 0, h = 0;
            getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h, &x_after);
            //printf("text: `%s` bounds: %d, %d\n", s.c_str(), x1, w);
            setCursor(x - x_after, y);
        }
        print(s.c_str());
    }

    void set_text_color(int r, int g, int b)
    {
        setTextColor(color565(r, g, b));
    }

    void set_font(int index)
    {
        if (index < 0) {
            setFont(nullptr);
            return;
        }
        if (index < 0 || index >= (sizeof(all_fonts) / sizeof(FontDef)))
            return;
        setFont(all_fonts[index].fontPtr);
    }
    int16_t calc_str_width(const std::string& s) {
        int16_t x1 = 0, y1 = 0, x_after = 0;
        uint16_t w = 0, h = 0;
        getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h, &x_after);
        return x_after;
    }

    void set_back_col(int r, int g, int b)
    {
        m_backcol = color565(r, g, b);
    }

    void clear()
    {
        fillScreen(m_backcol);
    }
    void finish()
    {
        m_drawer->flush();
    }

private:
    uint16_t m_backcol = 0;
    IDraw* m_drawer = nullptr; // owned by the panel
};


std::shared_ptr<JsDisplay> gfx_init_display(int w, int h)
{
    std::shared_ptr<JsDisplay> disp(new JsDisplay(w, h));
    disp->setTextWrap(false);

    //disp->print("Hello");
    return disp;
}

/*
std::shared_ptr<JsDraw> make_js_drawer(emscripten::val wrapped) {
    return std::make_shared<JsDraw>(wrapped);
}

std::shared_ptr<HaloDraw> make_halo_drawer(IDraw* wrap, int w, int h) {
    return std::make_shared<HaloDraw>(wrap, w, h);
}*/

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



// TODO support wider chars

EMSCRIPTEN_BINDINGS(my_module)
{
    using namespace emscripten;
    function("gfx_init_display", &gfx_init_display);
    //function("make_drawer", &make_drawer);
    register_vector<std::string>("StringList");
    register_vector<uint8_t>("UInt8List");
    function("gfx_get_fonts", &gfx_get_fonts);
    function("color888v", &color888v);
    function("color565", &color565);

    class_<IDraw>("IDraw")
        .smart_ptr<std::shared_ptr<IDraw>>("IDraw")
        .function("setPixel", &IDraw::setPixel)
        ;

    class_<JsDraw, base<IDraw>>("JsDraw")
        .smart_ptr_constructor("JsDraw", &std::make_shared<JsDraw, emscripten::val>)
        ;

    class_<HaloDraw, base<IDraw>>("HaloDraw")
        .smart_ptr_constructor("HaloDraw", &std::make_shared<HaloDraw, IDraw*, int, int>)
        .function("setMyColor", &HaloDraw::setMyColor)
        ;

    class_<JsDisplay>("JsDisplay")
        .smart_ptr<std::shared_ptr<JsDisplay>>("JsDisplay")
        .function("drawPixel", &JsDisplay::drawPixel)
        .function("print_str_at", &JsDisplay::print_str_at)
        .function("set_text_color", &JsDisplay::set_text_color)
        .function("set_font", &JsDisplay::set_font)
        .function("clear", &JsDisplay::clear)
        .function("finish", &JsDisplay::finish)
        .function("set_back_col", &JsDisplay::set_back_col)
        .function("calc_str_width", &JsDisplay::calc_str_width)
        .function("set_drawer", &JsDisplay::set_drawer, allow_raw_pointers())
        ;
    
}