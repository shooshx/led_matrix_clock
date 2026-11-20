#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <soc/i2s_struct.h>
#include "audio.h"

static const int AUDIO_PIN = A0;         // GPIO12
static const i2s_port_t I2S_PORT = I2S_NUM_0;  // only I2S0 can do PDM TX
static const int SAMPLE_RATE = 16000;
static const int BUF_FRAMES = 256;
static const int MAX_FREQ = SAMPLE_RATE / 2 - 1000;  // below the Nyquist frequency

static TaskHandle_t g_task = nullptr;
static volatile bool g_beep = false;
static volatile int g_freq = 440;
static volatile int g_volume = 30;  // percent of full scale, the amplifier is loud
static int16_t g_sine[256];

// while nothing plays the pin is a plain output held low: when it was left with I2S, or floating before the
// first sound, the amplifier made a hiss / buzz out of it
static void hold_pin_low()
{
    esp_rom_gpio_connect_out_signal(AUDIO_PIN, SIG_GPIO_OUT_IDX, false, false);  // away from I2S
    gpio_reset_pin((gpio_num_t)AUDIO_PIN);
    gpio_set_direction((gpio_num_t)AUDIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)AUDIO_PIN, 0);
}

// PDM has only a clock (on the WS pin, not needed by the speaker) and the data
static bool attach_pin()
{
    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = I2S_PIN_NO_CHANGE;
    pins.ws_io_num = I2S_PIN_NO_CHANGE;
    pins.data_out_num = AUDIO_PIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;
    esp_err_t err = i2s_set_pin(I2S_PORT, &pins);
    if (err != ESP_OK)
        Serial.printf("audio: i2s_set_pin failed %d\n", err);
    return err == ESP_OK;
}

// the pin's average level while nothing plays and at the ends of the ramps, that of a pin held low.
// silence (0) is half the supply voltage, the jump between the two clicked
static const int16_t LOW_LEVEL = -32767;
static const int RAMP_FRAMES = SAMPLE_RATE / 10;  // 100 ms between LOW_LEVEL and playing
static const int DMA_BUFS = 4;

static size_t write_buf(int16_t* buf)
{
    size_t written = 0;
    i2s_write(I2S_PORT, buf, BUF_FRAMES * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    return written;
}

// sends LOW_LEVEL until everything that was in the DMA buffers has played
static void flush_low(int16_t* buf)
{
    for (int i = 0; i < BUF_FRAMES * 2; ++i)
        buf[i] = LOW_LEVEL;
    for (int i = 0; i < DMA_BUFS * 2; ++i)
        write_buf(buf);
}

// fills the DMA while g_beep is set, waits for a notification while it isn't.
// starting: the stream runs at LOW_LEVEL before it gets the pin, then rises to silence and the beep fades in.
// stopping: the reverse, the pin is held low again once the stream is down at LOW_LEVEL
static void audio_task(void*)
{
    static int16_t buf[BUF_FRAMES * 2];  // stereo frames, the same sample in both
    uint32_t phase = 0;
    bool running = false;
    int ramp = 0;  // 0..RAMP_FRAMES
    for (;;) {
        if (!running) {
            if (!g_beep) {
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                continue;
            }
            i2s_start(I2S_PORT);
            flush_low(buf);
            attach_pin();
            running = true;
            ramp = 0;
            phase = 0;
            Serial.println("audio: playing");
        }

        // read once per buffer, a change is heard within a buffer (16 ms)
        const bool up = g_beep;
        const uint32_t phase_step = (uint32_t)((double)g_freq / SAMPLE_RATE * 4294967296.0);
        const float volume = g_volume / 100.0f;
        for (int i = 0; i < BUF_FRAMES; ++i) {
            if (up && ramp < RAMP_FRAMES)
                ++ramp;
            else if (!up && ramp > 0)
                --ramp;
            float x = (float)ramp / RAMP_FRAMES;
            float e = x * x * (3 - 2 * x);  // smoothstep, no sudden change at the ends
            int16_t s = (int16_t)(LOW_LEVEL * (1 - e) + g_sine[phase >> 24] * volume * e);
            buf[i * 2] = s;
            buf[i * 2 + 1] = s;
            phase += phase_step;
        }
        write_buf(buf);

        if (!up && ramp == 0) {
            flush_low(buf);
            hold_pin_low();
            i2s_stop(I2S_PORT);
            running = false;
            Serial.println("audio: stopped");
        }
    }
}

static bool audio_init()
{
    if (g_task)
        return true;

    for (int i = 0; i < 256; ++i)
        g_sine[i] = (int16_t)(sinf(i * 2 * PI / 256) * 32767);

    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM);
    cfg.sample_rate = SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = 0;
    cfg.dma_buf_count = DMA_BUFS;
    cfg.dma_buf_len = BUF_FRAMES;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;  // if the data runs out it repeats, zeros would be a jump to half the supply
    esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("audio: i2s_driver_install failed %d\n", err);
        return false;
    }

    // the driver turns on the PDM high pass filter, that would take away LOW_LEVEL and keep the pin
    // at half the supply voltage
    I2S0.tx_pcm2pdm_conf.tx_hp_bypass = 1;

    // installing starts it, the task starts it (and gives it the pin) when there's something to play
    i2s_stop(I2S_PORT);

    xTaskCreate(audio_task, "audio", 3072, nullptr, 3, &g_task);
    Serial.println("audio: initialized");
    return true;
}

void audio_begin()
{
    hold_pin_low();
}

void audio_beep(bool on)
{
    if (on && !audio_init())
        return;
    g_beep = on;
    if (g_task)
        xTaskNotifyGive(g_task);
}

void audio_set_freq(int hz)
{
    g_freq = constrain(hz, 20, MAX_FREQ);
}

void audio_set_volume(int percent)
{
    g_volume = constrain(percent, 0, 100);
}

bool audio_beeping()
{
    return g_beep;
}
