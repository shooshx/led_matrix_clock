
#include <iostream>

#include "Adafruit_GFX.h"

class ConsoleTest : public Adafruit_GFX
{
public:
    ConsoleTest() :Adafruit_GFX(64, 32)
    {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        std::cout << "draw " << x << "," << y << " : " << color << std::endl;
    }
};

int main()
{
    ConsoleTest disp;
    disp.print("Hello");

}

