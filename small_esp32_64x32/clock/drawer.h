#pragma once


class IDraw {
public:
    virtual ~IDraw() {} 
    virtual void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void clear() = 0;
    virtual void flush() = 0;
};


class RGBBuffer
{
public:
    RGBBuffer(int w, int h) : m_w(w), m_h(h) {
        m_buf = new uint8_t[w * h * 3];
    }
    ~RGBBuffer() {
        delete[] m_buf;
    }
    void set(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= m_w || y >= m_h)
            return;
        //std::cout << " buf " << x << "," << y << " - " << (int)r << "," << (int)g << "," << (int)b << std::endl;
        int i = (x + y*m_w)*3;
        m_buf[i] = r;
        m_buf[i+1] = g;
        m_buf[i+2] = b;
    }
    template<typename TF>
    void foreach(TF cb) {
        int i = 0;
        for(int y = 0; y < m_h; ++y) {
            for(int x = 0; x < m_w; ++x) {
                cb(x, y, m_buf[i], m_buf[i+1], m_buf[i+2]);
                i += 3;
            }
        }
    }
    void clear() {
        memset(m_buf, 0, m_w * m_h * 3);
    }

private:
    int m_w = 0;
    int m_h = 0;
    uint8_t* m_buf = nullptr;
};

class HaloDraw : public IDraw
{
public:
    HaloDraw(IDraw* wrap, int w, int h) : m_wrap(wrap), m_tmp(w, h)
    {}
    void setMyColor(uint8_t r, uint8_t g, uint8_t b) {
        m_r = r; m_g = g; m_b = b;
    }
    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override {
        if (r == 0 && g == 0 && b == 0) {
            m_wrap->setPixel(x, y, r, g, b);
            return;
        }

        //std::cout << " orig " << x << "," << y << " - " << (int)r << "," << (int)g << "," << (int)b << std::endl;
        m_tmp.set(x, y, r, g, b);
        m_wrap->setPixel(x + 1, y, m_r, m_g, m_b);
        m_wrap->setPixel(x - 1, y, m_r, m_g, m_b);
        m_wrap->setPixel(x, y + 1, m_r, m_g, m_b);
        m_wrap->setPixel(x, y - 1, m_r, m_g, m_b);
    }

    void clear() override {
        m_tmp.clear();
        m_wrap->clear();
    }
    
    void flush() override {
        int count = 0;
        m_tmp.foreach([this, &count](uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
            if (r == 0 && g == 0 && b == 0)
                return;
            //std::cout << " atflush " << x << "," << y << " - " << (int)r << "," << (int)g << "," << (int)b << std::endl;
            m_wrap->setPixel(x, y, r, g, b);
            ++count; 
        });
        //Serial.printf("halo flush %d\n", count);
        m_tmp.clear();
        m_wrap->flush();
    }
private:
    uint8_t m_r = 0, m_g = 0, m_b = 0;
    IDraw* m_wrap;
    RGBBuffer m_tmp;
};
