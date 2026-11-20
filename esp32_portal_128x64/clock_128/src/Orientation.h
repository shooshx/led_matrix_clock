#pragma once
#include <Wire.h>
#include <Adafruit_LIS3DH.h>

// Detects which side the panel stands on from the board's accelerometer,
// as an Adafruit_GFX rotation: 0 landscape, 2 landscape upside down, 1 and 3 portrait
class Orientation
{
public:
    static const unsigned long SAMPLE_MS = 500;
    static constexpr float MIN_DOWN = 6.0f; // m/s^2 needed on the down axis, less is lying flat or moving
    static const int STABLE_SAMPLES = 2;    // the same new side this many samples in a row before switching

    bool begin() {
        m_ok = m_accel.begin(0x19);
        if (!m_ok) {
            Serial.println("accelerometer not found, orientation is fixed");
            return false;
        }
        m_accel.setRange(LIS3DH_RANGE_4_G); // like the pixel dust demo, so shaking the panel registers
        return true;
    }

    int rotation() const { return m_rotation; }
    // a reading now, m/s^2 (for the sand game, every frame). false if there's no accelerometer
    bool read(float& x, float& y, float& z) {
        if (!m_ok)
            return false;
        sensors_event_t e;
        m_accel.getEvent(&e);
        x = m_ax = e.acceleration.x;
        y = m_ay = e.acceleration.y;
        z = m_az = e.acceleration.z;
        return true;
    }

    // the last raw reading, m/s^2
    bool found() const { return m_ok; }
    float ax() const { return m_ax; }
    float ay() const { return m_ay; }
    float az() const { return m_az; }

    // called in loop, returns true if the rotation changed
    bool update() {
        if (!m_ok || millis() - m_last_sample < SAMPLE_MS)
            return false;
        m_last_sample = millis();
        sensors_event_t e;
        m_accel.getEvent(&e);
        m_ax = e.acceleration.x;
        m_ay = e.acceleration.y;
        m_az = e.acceleration.z;
        int r = rotation_of(e.acceleration.x, e.acceleration.y);
        if (r < 0 || r == m_rotation) {
            m_candidate = -1;
            return false;
        }
        if (r != m_candidate) {
            m_candidate = r;
            m_count = 0;
        }
        // the first reading after boot is taken right away
        if (++m_count < STABLE_SAMPLES && !m_first)
            return false;
        Serial.printf("orientation: rotation %d (accel x=%.1f y=%.1f z=%.1f)\n", r,
                      e.acceleration.x, e.acceleration.y, e.acceleration.z);
        m_rotation = r;
        m_first = false;
        m_candidate = -1;
        return true;
    }

    // the pixeldust demo moves the sand along (accel x, accel y), so that's the direction of down
    // in panel coordinates (x right, y down). returns -1 if no side is clearly down
    static int rotation_of(float ax, float ay) {
        if (fabsf(ay) >= fabsf(ax)) {
            if (fabsf(ay) < MIN_DOWN)
                return -1;
            return ay > 0 ? 0 : 2;
        }
        if (fabsf(ax) < MIN_DOWN)
            return -1;
        // rotation 3 draws logical down towards panel +x, rotation 1 towards -x
        return ax > 0 ? 3 : 1;
    }

private:
    Adafruit_LIS3DH m_accel;
    bool m_ok = false;
    bool m_first = true;
    int m_rotation = 0;
    int m_candidate = -1;
    int m_count = 0;
    unsigned long m_last_sample = 0;
    float m_ax = 0, m_ay = 0, m_az = 0;
};
