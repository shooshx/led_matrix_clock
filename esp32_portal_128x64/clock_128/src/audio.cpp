#include <Arduino.h>
#include <algorithm>
#include <LittleFS.h>
#include <driver/i2s.h>
#include <MP3DecoderHelix.h>
#include "audio.h"

// the MAX98357A amplifier: I2S in, the speaker out. its SD pin isn't connected, it plays (left + right) / 2
static const int PIN_BCLK = A4;  // GPIO11
static const int PIN_LRC = A3;   // GPIO10, word select
static const int PIN_DIN = A2;   // GPIO9
static const i2s_port_t I2S_PORT = I2S_NUM_0;
static const int BEEP_RATE = 44100;
static const int BUF_FRAMES = 256;
static const int DMA_BUFS = 4;
static const int MAX_FREQ = BEEP_RATE / 2 - 2000;  // below the Nyquist frequency
static const int RAMP_MS = 20;                     // fade in / out, starting or stopping a wave mid-way clicks
static const char* SOUNDS_DIR = "/sounds";
static const size_t READ_CHUNK = 512;              // of an mp3 file, ~85 ms of 48 kbps

// ---- what the other tasks ask for ----
static TaskHandle_t g_task = nullptr;
static volatile bool g_beep = false;
static volatile int g_freq = 440;
static volatile int g_volume = 30;  // percent
struct PlayReq { char name[48]; };   // empty name: stop the file
static QueueHandle_t g_play_queue = nullptr;
static volatile bool g_playing_file = false;
static std::vector<String> g_sounds;

// ---- the audio task's own ----
enum Source { SRC_NONE, SRC_BEEP, SRC_FILE };
static int16_t g_sine[256];
static int16_t g_out[BUF_FRAMES * 2];  // stereo frames, the same sample in both
static int g_out_n = 0;                // frames in g_out
static int g_rate = BEEP_RATE;         // of I2S now
static float g_env = 0;                // the fade, 0..1
static float g_env_target = 0;
static float g_volume_now = 0;         // the amplitude of g_volume, read once a buffer
static libhelix::MP3DecoderHelix g_mp3;
static File g_file;

static void flush_out()
{
    if (g_out_n == 0)
        return;
    size_t written = 0;
    i2s_write(I2S_PORT, g_out, g_out_n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    g_out_n = 0;
}

// a sample to the speaker, at the volume and the fade
static void emit(int16_t s)
{
    if (g_env != g_env_target) {
        float step = 1000.0f / (RAMP_MS * g_rate);
        g_env = g_env < g_env_target ? min(g_env_target, g_env + step) : max(g_env_target, g_env - step);
    }
    float e = g_env * g_env * (3 - 2 * g_env);  // smoothstep, no sudden change at the ends
    int16_t v = (int16_t)(s * g_volume_now * e);
    g_out[g_out_n * 2] = v;
    g_out[g_out_n * 2 + 1] = v;
    if (++g_out_n == BUF_FRAMES)
        flush_out();
}

static void read_volume()
{
    // the ear hears loudness about logarithmically, squared makes the slider's steps more even
    float v = g_volume / 100.0f;
    g_volume_now = v * v;
}

static void set_rate(int rate)
{
    if (rate == g_rate || rate <= 0)
        return;
    flush_out();
    i2s_set_sample_rates(I2S_PORT, rate);
    g_rate = rate;
}

// the decoded samples of the mp3
static void on_mp3_data(MP3FrameInfo& info, short* pcm, size_t len, void*)
{
    set_rate(info.samprate);
    read_volume();
    if (info.nChans == 2) {
        for (size_t i = 0; i + 1 < len; i += 2)
            emit((int16_t)(((int)pcm[i] + pcm[i + 1]) / 2));
    }
    else {
        for (size_t i = 0; i < len; ++i)
            emit(pcm[i]);
    }
}

static bool open_file(const char* name)
{
    String path = String(SOUNDS_DIR) + "/" + name;
    g_file = LittleFS.open(path, "r");
    if (!g_file) {
        Serial.printf("audio: can't open %s\n", path.c_str());
        return false;
    }
    g_mp3.setDataCallback(on_mp3_data);
    if (!g_mp3.begin()) {
        Serial.println("audio: mp3 decoder failed to start");
        g_file.close();
        return false;
    }
    Serial.printf("audio: playing %s, %u bytes\n", path.c_str(), (unsigned)g_file.size());
    return true;
}

static void close_file()
{
    if (g_file)
        g_file.close();
    g_mp3.end();
}

// one source at a time: a new one fades the current out first. while nothing plays I2S is stopped: without the
// clocks the amplifier shuts down by itself, no hiss
static void audio_task(void*)
{
    Source src = SRC_NONE;
    Source next = SRC_NONE;     // to start once the current one faded out
    bool has_next = false;
    char next_name[48] = "";
    bool running = false;       // I2S
    uint32_t phase = 0;
    static uint8_t chunk[READ_CHUNK];

    for (;;) {
        // what's asked for
        PlayReq req;
        if (xQueueReceive(g_play_queue, &req, 0) == pdTRUE) {
            has_next = true;
            next = req.name[0] ? SRC_FILE : SRC_NONE;
            strlcpy(next_name, req.name, sizeof(next_name));
        }
        else if (g_beep && src != SRC_BEEP && !(has_next && next == SRC_BEEP)) {
            has_next = true;
            next = SRC_BEEP;
        }
        else if (!g_beep && ((src == SRC_BEEP && !has_next) || (has_next && next == SRC_BEEP))) {
            has_next = true;
            next = SRC_NONE;
        }

        // the current one fades out before the next
        if (has_next && src != SRC_NONE) {
            g_env_target = 0;
            if (g_env == 0) {
                if (src == SRC_FILE)
                    close_file();
                src = SRC_NONE;
            }
        }
        if (has_next && src == SRC_NONE) {
            has_next = false;
            if (next != SRC_NONE) {
                if (!running) {
                    i2s_zero_dma_buffer(I2S_PORT);
                    i2s_start(I2S_PORT);
                    running = true;
                }
                g_env = 0;
                g_env_target = 1;
                if (next == SRC_BEEP) {
                    set_rate(BEEP_RATE);
                    phase = 0;
                    src = SRC_BEEP;
                    Serial.println("audio: beep");
                }
                else if (open_file(next_name)) {
                    src = SRC_FILE;
                }
            }
        }
        g_playing_file = src == SRC_FILE || (has_next && next == SRC_FILE);

        if (src == SRC_NONE) {
            if (running) {
                // silence until what's in the DMA has played, then the clocks stop
                flush_out();
                memset(g_out, 0, sizeof(g_out));
                for (int i = 0; i < DMA_BUFS + 1; ++i) {
                    g_out_n = BUF_FRAMES;
                    flush_out();
                }
                i2s_stop(I2S_PORT);
                running = false;
                Serial.println("audio: stopped");
            }
            if (!has_next)
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        if (src == SRC_BEEP) {
            read_volume();
            const uint32_t phase_step = (uint32_t)((double)g_freq / g_rate * 4294967296.0);
            for (int i = 0; i < BUF_FRAMES; ++i) {
                emit(g_sine[phase >> 24]);
                phase += phase_step;
            }
        }
        else {
            size_t n = g_file.read(chunk, sizeof(chunk));
            if (n > 0)
                g_mp3.write(chunk, n);
            else {
                // the end of the file, what the decoder still has
                g_mp3.flush();
                close_file();
                flush_out();
                src = SRC_NONE;
                g_env = 0;
                Serial.println("audio: file ended");
            }
        }
    }
}

static bool audio_init()
{
    if (g_task)
        return true;
    size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t block_before = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    for (int i = 0; i < 256; ++i)
        g_sine[i] = (int16_t)(sinf(i * 2 * PI / 256) * 32767);

    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = BEEP_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = 0;
    cfg.dma_buf_count = DMA_BUFS;
    cfg.dma_buf_len = BUF_FRAMES;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = true;  // if the data runs out, silence
    esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("audio: i2s_driver_install failed %d\n", err);
        return false;
    }
    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;  // the MAX98357A makes its own clock from BCLK
    pins.bck_io_num = PIN_BCLK;
    pins.ws_io_num = PIN_LRC;
    pins.data_out_num = PIN_DIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;
    err = i2s_set_pin(I2S_PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("audio: i2s_set_pin failed %d\n", err);
        return false;
    }
    g_rate = BEEP_RATE;

    // installing starts it, the task starts it when there's something to play
    i2s_stop(I2S_PORT);

    g_play_queue = xQueueCreate(1, sizeof(PlayReq));
    xTaskCreate(audio_task, "audio", 6144, nullptr, 3, &g_task);
    // the I2S DMA buffers and the task's stack are in the internal RAM, which the web server and WiFi need too
    Serial.printf("audio: initialized, internal heap %u -> %u, largest block %u -> %u\n",
                  (unsigned)heap_before, (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)block_before, (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    return true;
}

void audio_begin()
{
    // the sounds in the file system, the file system is mounted before
    g_sounds.clear();
    File dir = LittleFS.open(SOUNDS_DIR);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("audio: no %s folder\n", SOUNDS_DIR);
        return;
    }
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        String name = f.name();
        int slash = name.lastIndexOf('/');  // some versions give the path
        if (slash >= 0)
            name = name.substring(slash + 1);
        if (!f.isDirectory() && name.endsWith(".mp3"))
            g_sounds.push_back(name);
    }
    dir.close();
    std::sort(g_sounds.begin(), g_sounds.end(), [](const String& a, const String& b) { return a < b; });
    Serial.printf("audio: %d sounds in %s\n", (int)g_sounds.size(), SOUNDS_DIR);
    for (const String& s : g_sounds)
        Serial.printf("  %s\n", s.c_str());
}

const std::vector<String>& audio_sounds()
{
    return g_sounds;
}

void audio_beep(bool on)
{
    if (on && g_volume == 0) {  // nothing to hear, nothing starts (I2S and the amplifier stay off)
        Serial.println("audio: volume 0, no beep");
        return;
    }
    if (on && !audio_init())
        return;
    g_beep = on;
    if (g_task)
        xTaskNotifyGive(g_task);
}

void audio_play(const char* name)
{
    if (name && name[0] && g_volume == 0) {  // nothing to hear, nothing starts. stopping ("") still does
        Serial.printf("audio: volume 0, %s not played\n", name);
        return;
    }
    if (!audio_init())
        return;
    PlayReq req;
    strlcpy(req.name, name ? name : "", sizeof(req.name));
    if (req.name[0])
        g_beep = false;  // one at a time
    xQueueOverwrite(g_play_queue, &req);
    xTaskNotifyGive(g_task);
}

void audio_stop()
{
    g_beep = false;
    if (g_task)
        audio_play("");
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

bool audio_playing_file()
{
    return g_playing_file;
}
