#pragma once
// included by ClockState.h, after ClockLayout

// ---- the Random mode of a clock face, same as random_face.js in the web-ui ----
// every aspect of the face is by chance: which texts show the time and the date, their fonts, sizes, colors,
// outlines, where they are, and the background effect. what has to stay: the time and the date are shown,
// fully on the panel, not over each other, legible, and in their order: the texts are placed in reading
// order (lines from the top, left to right in a line), hour before minute before second, weekday before
// day / month / year.
// the texts are measured with the display's measuring mode (WrapGFX::measure_begin(), the boxes of the glyphs)
// over every value they can show, so that they fit whatever the time shows. the boxes of the glyphs hold their
// pixels, so it's a bit more careful than the web-ui, that measures the pixels

const int32_t RND_INTERVAL_DEF = 300;        // seconds between faces
const int32_t RND_INTERVAL_MIN = 1;
const int32_t RND_INTERVAL_MAX = 24 * 3600;
const int RND_ATTEMPTS = 80;                 // random faces tried before the simple fallback one
const int RND_MIN_DIGIT_H = 5;               // the time's digits at least this high, smaller isn't legible
const int RND_FONTS = sizeof(all_fonts) / sizeof(all_fonts[0]);

inline uint32_t rnd_u32() { return esp_random(); }
inline int rnd_int(int a, int b) { return a + (int)(rnd_u32() % (uint32_t)(b - a + 1)); }  // a..b
inline float rnd_float() { return (rnd_u32() >> 8) / 16777216.0f; }                       // 0..1
inline bool rnd_chance(float p) { return rnd_float() < p; }
// an index by weights
inline int rnd_weighted(const float* w, int n) {
    float sum = 0;
    for (int i = 0; i < n; ++i)
        sum += w[i];
    float t = rnd_float() * sum;
    for (int i = 0; i < n; ++i) {
        t -= w[i];
        if (t < 0)
            return i;
    }
    return n - 1;
}

// h 0-360, s and v 0-1, as rgb 0-255
inline void hsv_rgb(int h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    h = ((h % 360) + 360) % 360;
    float c = v * s, x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1)), m = v - c;
    float rr, gg, bb;
    if (h < 60)       { rr = c; gg = x; bb = 0; }
    else if (h < 120) { rr = x; gg = c; bb = 0; }
    else if (h < 180) { rr = 0; gg = c; bb = x; }
    else if (h < 240) { rr = 0; gg = x; bb = c; }
    else if (h < 300) { rr = x; gg = 0; bb = c; }
    else              { rr = c; gg = 0; bb = x; }
    r = (uint8_t)roundf((rr + m) * 255);
    g = (uint8_t)roundf((gg + m) * 255);
    b = (uint8_t)roundf((bb + m) * 255);
}
inline uint16_t hsv565(int h, float s, float v) {
    uint8_t r, g, b;
    hsv_rgb(h, s, v, r, g, b);
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
inline int hue_dist(int a, int b) {
    int d = abs(((a - b) % 360 + 360) % 360);
    return min(d, 360 - d);
}

// the texts a random face can show
enum RndText { RT_HMS, RT_HM, RT_DATE, RT_HOUR, RT_MIN, RT_SEC, RT_WDAY, RT_DAY, RT_MONTH, RT_YEAR, RT_COUNT };

struct RndBox {
    int16_t x0 = 0, y0 = 0, x1 = -1, y1 = -1;
    bool ok() const { return x1 >= x0; }
    int w() const { return x1 - x0 + 1; }
    int h() const { return y1 - y0 + 1; }
    void add(int16_t ax0, int16_t ay0, int16_t ax1, int16_t ay1) {
        if (!ok()) {
            x0 = ax0; y0 = ay0; x1 = ax1; y1 = ay1;
            return;
        }
        x0 = min(x0, ax0); y0 = min(y0, ay0);
        x1 = max(x1, ax1); y1 = max(y1, ay1);
    }
};

// the background effect of a random face
struct RndFx {
    int16_t fx = FX_NONE;
    uint16_t color = 0;
    int16_t size = 1;
    int16_t speed = 1;
    bool pulse = false;
    int16_t angle = 0;
    int hue = 0;
    bool animated() const { return fx_animated(fx, pulse); }
};

// the texts of a random face: a ClockLayout and the time without seconds, not saved (not in the names)
struct RandomLayout : public PropHolder<2>
{
    ClockLayout m_l;
    TimeTextBlock m_hm;

    RandomLayout()
      : PropHolder((NamesIndex*)nullptr)
      , m_l(this, "rnd", false)
      , m_hm(this, "rndh", false)
    {}

    TextBlock* block(int k) {
        switch (k) {
            case RT_HMS:   return &m_l.m_time_text;
            case RT_HM:    return &m_hm;
            case RT_DATE:  return &m_l.m_date_text;
            case RT_HOUR:  return &m_l.m_hour;
            case RT_MIN:   return &m_l.m_min;
            case RT_SEC:   return &m_l.m_sec;
            case RT_WDAY:  return &m_l.m_weekday;
            case RT_DAY:   return &m_l.m_day;
            case RT_MONTH: return &m_l.m_month;
            default:       return &m_l.m_year;
        }
    }
    // each by its type, draw() isn't virtual
    void draw_text(int k) {
        switch (k) {
            case RT_HMS:  m_l.m_time_text.draw(); break;
            case RT_HM:   m_hm.draw(); break;
            case RT_DATE: m_l.m_date_text.draw(); break;
            default:      static_cast<FieldTextBlock*>(block(k))->draw(); break;
        }
    }
    void draw(time_t local) {
        m_l.draw_texts(local);
        m_hm.set_time(hour(local), minute(local), second(local), false);
        m_hm.draw();
    }
    // all hidden, no outline: before a new face is made on it
    void reset() {
        for (int k = 0; k < RT_COUNT; ++k) {
            block(k)->m_show.set(false);
            block(k)->m_outline.set(false);
        }
        m_l.m_back_color.set(0);
    }
};

// local time_t of a date
inline time_t rnd_make_time(int y, int mo, int d, int h, int mi, int s) {
    tmElements_t tm;
    tm.Year = y - 1970; tm.Month = mo; tm.Day = d; tm.Hour = h; tm.Minute = mi; tm.Second = s;
    return makeTime(tm);
}

// the boxes of the texts in the fonts, relative to the text's x, y. measured once for each look
class RndMeasure
{
public:
    static const int16_t AT = 300;

    // its box without the outline, not ok() if it draws nothing (a font without these characters)
    RndBox measure(RandomLayout& rl, int k) {
        TextBlock* b = rl.block(k);
        int font = b->m_font_index.get();
        if (font < -1 || font >= RND_FONTS)
            font = -1;
        int sd = (k == RT_DATE && rl.m_l.m_show_day.get()) ? 1 : 0;
        int i = ((k * (RND_FONTS + 1) + font + 1) * 2 + (b->m_double_size.get() ? 1 : 0)) * 2 + sd;
        if (m_cache == nullptr) {
            // ~16K, in PSRAM
            size_t n = (size_t)RT_COUNT * (RND_FONTS + 1) * 2 * 2;
            m_cache = (Entry*)heap_caps_calloc(n, sizeof(Entry), MALLOC_CAP_SPIRAM);
            if (m_cache == nullptr)
                m_cache = (Entry*)calloc(n, sizeof(Entry));
        }
        if (m_cache != nullptr && m_cache[i].measured)
            return m_cache[i].box;
        RndBox box = measure_now(rl, k);
        if (m_cache != nullptr) {
            m_cache[i].box = box;
            m_cache[i].measured = true;
        }
        return box;
    }

private:
    struct Entry { RndBox box; bool measured; };
    Entry* m_cache = nullptr;

    RndBox measure_now(RandomLayout& rl, int k) {
        TextBlock* b = rl.block(k);
        int16_t ox = b->m_x.get(), oy = b->m_y.get();
        bool oshow = b->m_show.get(), ooutline = b->m_outline.get();
        IDraw* drawer = display.drawer();
        b->m_x.set(AT);
        b->m_y.set(AT);
        b->m_show.set(true);
        b->m_outline.set(false);
        RndBox all;
        auto one = [&]() {
            int16_t x0, y0, x1, y1;
            display.measure_begin();
            rl.draw_text(k);
            if (display.measure_end(x0, y0, x1, y1))
                all.add(x0, y0, x1, y1);
        };
        // every value it can show: a digit can go further than the others in some fonts
        if (k == RT_HMS || k == RT_HM) {
            // every hour (its left), every last digit (its right). the slots fix what's between
            TimeTextBlock* t = static_cast<TimeTextBlock*>(b);
            bool sec = (k == RT_HMS);
            for (int h = 0; h < 24; ++h) {
                t->set_time(h, 0, 0, sec);
                one();
            }
            for (int u = 0; u < 10; ++u) {
                t->set_time(0, 50 + u, 50 + u, sec);
                one();
            }
        }
        else if (k == RT_DATE) {
            // the date line: dates with every weekday and every digit in every place, its widest, its leftmost
            for (time_t d : sample_dates()) {
                rl.m_l.set_time(d);
                one();
            }
            b->set(widest_date(rl));
            one();
            b->set(leftmost_date(rl));
            one();
        }
        else {
            int lo = 0, hi = 0;
            switch (k) {
                case RT_HOUR:  lo = 0; hi = 23; break;
                case RT_MIN:   lo = 0; hi = 59; break;
                case RT_SEC:   lo = 0; hi = 59; break;
                case RT_WDAY:  lo = 0; hi = 6; break;
                case RT_DAY:   lo = 1; hi = 31; break;
                case RT_MONTH: lo = 1; hi = 12; break;
                default:       lo = 2020; hi = 2039; break;
            }
            FieldTextBlock* f = static_cast<FieldTextBlock*>(b);
            for (int v = lo; v <= hi; ++v) {
                f->set_value(v);
                one();
            }
        }
        b->m_x.set(ox);
        b->m_y.set(oy);
        b->m_show.set(oshow);
        b->m_outline.set(ooutline);
        display.setDrawer(drawer);
        if (all.ok()) {
            all.x0 -= AT; all.x1 -= AT;
            all.y0 -= AT; all.y1 -= AT;
        }
        return all;
    }

    // every digit in every place of the day, month and year (2020-2039), and every weekday
    static std::vector<time_t> sample_dates() {
        static const int days[] = {30, 31, 12, 23, 14, 5, 16, 27, 18, 9};
        static const int months[] = {10, 12, 11, 3, 4, 5, 6, 7, 8, 9};
        std::vector<time_t> v;
        for (int i = 0; i < 10; ++i)
            v.push_back(rnd_make_time(2020 + i + (i % 2) * 10, months[i], days[i], 0, 0, 0));
        for (int d = 22; d <= 28; ++d)  // a week, all the weekdays
            v.push_back(rnd_make_time(2026, 11, d, 0, 0, 0));
        return v;
    }

    // the date line as wide as it gets: the widest weekday, day and month (by their advance), and the year
    // whose pixels go furthest right (the last glyph can go past its advance)
    String widest_date(RandomLayout& rl) {
        TextBlock* b = &rl.m_l.m_date_text;
        char buf[8];
        String year = "2026";
        int16_t year_x1 = -0x7FFF;
        for (int y = 2020; y <= 2039; ++y) {
            b->set(String(y));
            int16_t x0, y0, x1, y1;
            display.measure_begin();
            b->draw();
            if (display.measure_end(x0, y0, x1, y1) && x1 > year_x1) {
                year_x1 = x1;
                year = String(y);
            }
        }
        b->do_set_font();
        auto widest_num = [&](int lo, int hi) {
            int best = lo, best_w = -1;
            for (int v = lo; v <= hi; ++v) {
                snprintf(buf, sizeof(buf), "%d", v);
                int w = calc_str_width(buf);
                if (w > best_w) { best_w = w; best = v; }
            }
            return String(best);
        };
        String t;
        if (rl.m_l.m_show_day.get()) {
            int best = 0, best_w = -1;
            for (int d = 0; d < 7; ++d) {
                int w = calc_str_width(short_days[d]);
                if (w > best_w) { best_w = w; best = d; }
            }
            t = String(short_days[best]) + " ";
        }
        return t + widest_num(1, 31) + "/" + widest_num(1, 12) + "/" + year;
    }

    // the date line that goes furthest left: it starts with the day (no leading 0) whose pixels go furthest
    // left. with the weekday it starts with that, all of them are in the sample dates
    String leftmost_date(RandomLayout& rl) {
        TextBlock* b = &rl.m_l.m_date_text;
        String day = "1";
        int16_t day_x0 = 0x7FFF;
        for (int d = 1; d <= 31; ++d) {
            b->set(String(d));
            int16_t x0, y0, x1, y1;
            display.measure_begin();
            b->draw();
            if (display.measure_end(x0, y0, x1, y1) && x0 < day_x0) {
                day_x0 = x0;
                day = String(d);
            }
        }
        return String(rl.m_l.m_show_day.get() ? "Sun " : "") + day + "/1/2026";
    }
};

// a saturation by chance, mostly a strong one, sometimes from lo up (the paler ones are nice too)
inline float rnd_saturation(float lo)
{
    return rnd_chance(0.7f) ? 0.85f + rnd_float() * 0.15f : lo + rnd_float() * (1 - lo);
}

// the background effect of a random face
inline RndFx random_effect()
{
    RndFx fx;
    // none and the simple ones more often, the full-screen ones (aurora, fire, radar, plasma, lava, tunnel) less.
    // by the effect's number, like random_face.js
    float kinds[FX_COUNT];
    kinds[FX_NONE] = 4; kinds[FX_BORDER] = 3; kinds[FX_SLIDE] = 2;
    for (int e : {FX_TWINKLE, FX_FIREFLIES, FX_RAIN, FX_SNOW, FX_RIPPLES, FX_WARP})
        kinds[e] = 1.5f;
    for (int e : {FX_AURORA, FX_FIRE, FX_RADAR, FX_PLASMA, FX_LAVA, FX_TUNNEL, FX_CUBE})
        kinds[e] = 1;
    kinds[FX_SKY_REMOVED] = 0;
    fx.fx = rnd_weighted(kinds, FX_COUNT);
    fx.hue = rnd_int(0, 359);
    // 2-64, the small ones more often
    float s = rnd_float();
    fx.size = 2 + (int)roundf(s * s * (FX_SIZE_MAX - 2));
    fx.speed = rnd_int(1, 10);
    fx.pulse = fx_can_pulse(fx.fx) && rnd_chance(0.5f);
    // mostly straight across, sometimes slanted
    fx.angle = rnd_chance(0.5f) ? (rnd_chance(0.5f) ? 0 : 90) : rnd_int(0, 179);
    fx.color = hsv565(fx.hue, rnd_saturation(0.6f), 0.5f + rnd_float() * 0.5f);
    return fx;
}

// a face by chance on rl for a panel of w x h, false if what came out doesn't fit
inline bool try_random_face(RandomLayout& rl, RndFx& fx, RndMeasure& m, int w, int h, float shrink)
{
    rl.reset();
    fx = random_effect();

    // the texts of the time and of the date, each in its order
    int time[3], nt = 0, date[4], nd = 0;
    const float tw[] = {3, 3, 3};  // h:mm:ss, h:mm, fields
    int tk = rnd_weighted(tw, 3);
    if (tk == 0)
        time[nt++] = RT_HMS;
    else if (tk == 1)
        time[nt++] = RT_HM;
    else {
        time[nt++] = RT_HOUR;
        time[nt++] = RT_MIN;
        if (rnd_chance(0.5f))
            time[nt++] = RT_SEC;
    }
    if (rnd_chance(0.6f)) {
        date[nd++] = RT_DATE;
        rl.m_l.m_show_day.set(rnd_chance(0.5f));  // the date line with the weekday
    }
    else {
        if (rnd_chance(0.5f))
            date[nd++] = RT_WDAY;
        date[nd++] = RT_DAY;
        date[nd++] = RT_MONTH;
        date[nd++] = RT_YEAR;
    }

    // the look: a font for each group (sometimes for each text), 2x, colors, outlines
    auto pick_font = [&]() {
        if (rnd_chance(shrink))
            return -1;  // the default font is small and legible
        return rnd_int(-1, RND_FONTS - 1);
    };
    const float sw[] = {2, 3, 2};  // one color, a color for each group, for each text
    int scheme = rnd_weighted(sw, 3);
    int base_hue = rnd_int(0, 359);
    const int group_steps[] = {120, 150, 180};
    int group_step = group_steps[rnd_int(0, 2)];
    // with an effect behind them the texts have a dark outline, legible over it. otherwise sometimes
    bool outline = fx.fx != FX_NONE || rnd_chance(0.3f);
    uint16_t outline_color = rnd_chance(0.5f) ? 0 : hsv565(base_hue + 180, 0.8f, 0.25f);

    int item_index = 0;
    for (int gi = 0; gi < 2; ++gi) {
        const int* group = gi == 0 ? time : date;
        int n = gi == 0 ? nt : nd;
        int group_font = pick_font();
        bool dbl = rnd_chance((gi == 0 ? 0.45f : 0.15f) * (1 - shrink));
        for (int i = 0; i < n; ++i) {
            TextBlock* b = rl.block(group[i]);
            b->m_font_index.set(rnd_chance(0.25f) ? pick_font() : group_font);
            b->m_double_size.set(dbl);
            int hue = base_hue;
            if (scheme == 1)
                hue += gi * group_step;
            else if (scheme == 2)
                hue += item_index * rnd_int(40, 90);
            // not the effect's color, the text stands out from it
            if (fx.fx != FX_NONE && hue_dist(hue, fx.hue) < 60)
                hue += 180;
            b->m_color.set(hsv565(hue, rnd_saturation(0.3f), 1.0f));
            b->m_outline.set(outline);
            b->m_outline_color.set(outline_color);
            ++item_index;
        }
    }

    // measured: drawn, legible, not bigger than the panel. the outline is a pixel around
    RndBox box[RT_COUNT];
    for (int gi = 0; gi < 2; ++gi) {
        const int* group = gi == 0 ? time : date;
        int n = gi == 0 ? nt : nd;
        for (int i = 0; i < n; ++i) {
            int k = group[i];
            RndBox bx = m.measure(rl, k);
            if (!bx.ok())
                return false;
            if (gi == 0 && bx.h() < RND_MIN_DIGIT_H * (rl.block(k)->m_double_size.get() ? 2 : 1))
                return false;
            if (outline) {
                --bx.x0; --bx.y0;
                ++bx.x1; ++bx.y1;
            }
            if (bx.w() > w || bx.h() > h)
                return false;
            box[k] = bx;
        }
    }

    // in reading order: mostly the time first
    int seq[7], ns = 0;
    bool time_first = rnd_chance(0.8f);
    for (int gi = 0; gi < 2; ++gi) {
        bool is_time = (gi == 0) == time_first;
        const int* group = is_time ? time : date;
        int n = is_time ? nt : nd;
        for (int i = 0; i < n; ++i)
            seq[ns++] = group[i];
    }
    auto is_time_text = [&](int k) {
        for (int i = 0; i < nt; ++i)
            if (time[i] == k)
                return true;
        return false;
    };
    // into lines: a break between texts by chance, more likely between the time and the date
    std::vector<std::vector<int>> lines;
    lines.push_back({ seq[0] });
    for (int i = 1; i < ns; ++i) {
        bool same_group = is_time_text(seq[i]) == is_time_text(seq[i - 1]);
        if (rnd_chance(same_group ? 0.3f : 0.75f))
            lines.push_back({ seq[i] });
        else
            lines.back().push_back(seq[i]);
    }
    // the spaces between texts in a line, and lines that are too wide broken further
    int gap = rnd_int(2, 7);
    auto line_w = [&](const std::vector<int>& line) {
        int s = gap * ((int)line.size() - 1);
        for (int k : line)
            s += box[k].w();
        return s;
    };
    for (size_t i = 0; i < lines.size(); ++i) {
        // the texts at its end go to a new line after it, that is checked next
        std::vector<int> carry;
        while (line_w(lines[i]) > w) {
            if (lines[i].size() == 1)
                return false;  // a single text wider than the panel
            carry.insert(carry.begin(), lines[i].back());
            lines[i].pop_back();
        }
        if (!carry.empty())
            lines.insert(lines.begin() + i + 1, carry);
    }
    int nl = lines.size();
    std::vector<int> heights;
    int total = 0;
    for (auto& line : lines) {
        int lh = 0;
        for (int k : line)
            lh = max(lh, box[k].h());
        heights.push_back(lh);
        total += lh;
    }
    if (total + (nl - 1) > h)
        return false;

    // the room that's left between the lines, at the top and at the bottom, by chance
    int free = h - total - (nl - 1);
    std::vector<float> weights;
    float wsum = 0;
    const float edge[] = {0, 0.5f, 1};
    for (int i = 0; i <= nl; ++i) {
        float x = rnd_float() + ((i == 0 || i == nl) ? edge[rnd_int(0, 2)] : 0.2f);
        weights.push_back(x);
        wsum += x;
    }
    std::vector<int> spaces;
    for (float x : weights)
        spaces.push_back((int)floorf(free * x / wsum));
    int v_align = rnd_int(0, 2);  // bottom, center, top
    const int h_aligns[] = {0, 1, 2, 3, 1};  // left, center, right, anywhere, center again
    int h_align = h_aligns[rnd_int(0, 4)];

    int y = spaces[0];
    for (int li = 0; li < nl; ++li) {
        const std::vector<int>& line = lines[li];
        int lw = line_w(line), lh = heights[li];
        int room = w - lw;
        int x = h_align == 0 ? 0 : h_align == 2 ? room : h_align == 1 ? room / 2 : rnd_int(0, room);
        for (int k : line) {
            const RndBox& bx = box[k];
            int top = y + (v_align == 2 ? 0 : v_align == 1 ? (lh - bx.h()) / 2 : lh - bx.h());
            // the block's x, y is where its box starts less the box's offset from them
            TextBlock* b = rl.block(k);
            b->m_x.set(x - bx.x0);
            b->m_y.set(top - bx.y0);
            b->m_show.set(true);
            x += bx.w() + gap;
        }
        y += lh + 1 + spaces[li + 1];
    }
    return true;
}

// when nothing by chance fits: the time and the date line in the default font, one under the other
inline void fallback_random_face(RandomLayout& rl, RndFx& fx, RndMeasure& m, int w, int h)
{
    rl.reset();
    fx = RndFx();
    rl.m_l.m_show_day.set(false);
    uint16_t c = hsv565(rnd_int(0, 359), 0.5f, 1.0f);
    const int ks[] = {RT_HMS, RT_DATE};
    RndBox boxes[2];
    int total = 2;
    for (int i = 0; i < 2; ++i) {
        TextBlock* b = rl.block(ks[i]);
        b->m_font_index.set(-1);
        b->m_double_size.set(false);
        b->m_color.set(c);
        boxes[i] = m.measure(rl, ks[i]);
        if (boxes[i].ok())
            total += boxes[i].h();
    }
    int y = max(0, (h - total) / 2);
    for (int i = 0; i < 2; ++i) {
        if (!boxes[i].ok())
            continue;
        TextBlock* b = rl.block(ks[i]);
        b->m_x.set((w - boxes[i].w()) / 2 - boxes[i].x0);
        b->m_y.set(y - boxes[i].y0);
        b->m_show.set(true);
        y += boxes[i].h() + 2;
    }
}

// a random face for a panel of w x h on rl
inline void make_random_face(RandomLayout& rl, RndFx& fx, RndMeasure& m, int w, int h)
{
    for (int attempt = 0; attempt < RND_ATTEMPTS; ++attempt) {
        // the later attempts go for smaller texts, more of them fit
        if (try_random_face(rl, fx, m, w, h, (float)attempt / RND_ATTEMPTS))
            return;
    }
    fallback_random_face(rl, fx, m, w, h);
}
