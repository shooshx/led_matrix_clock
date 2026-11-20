#pragma once
#include "Prop.h"
#include "base_utils.h"

extern WrapGFX display;
extern MatrixDrawer matrix_drawer;

struct Rect
{
  int16_t x1 = 0, y1 = 0;
  uint16_t w = 0, h = 0;
};

enum TextAlign {
  LEFT_ALIGN = 0,
  RIGHT_ALIGN = 1
};

template<int NParentPropNum = 0>
struct TextBlockN : public PropHolder<8 + NParentPropNum>
{
    String m_name;
    String m_text;
    Prop<int16_t> m_font_index;
    Prop<int16_t> m_x;
    Prop<int16_t> m_y;
    Prop<uint16_t> m_color;
    Prop<bool> m_outline;
    Prop<uint16_t> m_outline_color;
    Prop<bool> m_double_size; // every font pixel is 2x2
    Prop<bool> m_show;
    TextAlign m_align = LEFT_ALIGN;

    TextBlockN(IPropHolder* parent, const String& name, TextAlign align = LEFT_ALIGN, bool show = true)
      : PropHolder<8 + NParentPropNum>(parent), m_name(name)
      , m_font_index(this, name + "_font_idx", -1)
      , m_x(this, name + "_x", 0)
      , m_y(this, name + "_y", 0)
      , m_color(this, name + "_color", 0xffff)
      , m_outline(this, name + "_outline", false)
      , m_outline_color(this, name + "_outline_color", 0)
      , m_double_size(this, name + "_dbl", false)
      , m_show(this, name + "_show", show)
      , m_align(align)
    {
    }

    // initial position, for the defaults of a layout
    void init(int x, int y, bool dbl = false) {
        m_x.set(x);
        m_y.set(y);
        m_double_size.set(dbl);
    }

    void set(const String& text) {
        m_text = text;
    }

    // outlined text goes through the halo drawer that draws the outline immediately and
    // keeps the text itself until flush so that it's on top of all outlines
    void begin_draw() {
        if (m_outline.get()) {
            uint8_t r, g, b;
            color888(m_outline_color.get(), r, g, b);
            halo_drawer.setMyColor(r, g, b);
            display.setDrawer(&halo_drawer);
        }
        else {
            display.setDrawer(&matrix_drawer);
        }
    }
    // leave the halo drawer set so that display.finish() flushes the kept text
    void end_draw() {
        display.setDrawer(&halo_drawer);
    }

    void do_set_font() {
        if (m_font_index.get() < 0 || m_font_index.get() >= (int)(sizeof(all_fonts)/sizeof(all_fonts[0])) )
            display.setFont(nullptr);
        else
            display.setFont(all_fonts[m_font_index.get()].fontPtr);
        display.setTextColor(m_color.get());
        display.setTextSize(text_size());
    }
    int text_size() const {
        return m_double_size.get() ? 2 : 1;
    }
     
    void draw()
    {
        if (!m_show.get())
            return;
        Timer t;
        begin_draw();
        do_set_font();

        if (m_align == RIGHT_ALIGN) {
          Rect r;
          display.getTextBounds(m_text, m_x.get(), m_y.get(), &r.x1, &r.y1, &r.w, &r.h);
          display.setCursor(m_x.get() - (r.w + r.x1), m_y.get());
        }
        else {
          display.setCursor(m_x.get(), m_y.get());
          //Serial.printf("t: %d %d %s\n", m_x.get(), m_y.get(), m_text.c_str());
        }
        
        auto ts = t.restart();
        display.print(m_text);

        auto tb = t.restart();
        //printf("    text took %d, %d\n", ts, tb);
        end_draw();
    }
};

using TextBlock = TextBlockN<0>;


void print_str_at(int x, int y, const char* s)
{
  display.setCursor(x, y);
  display.print(s);
}

void print_str_at(int x, int y, const std::string& s)
{
  print_str_at(x, y, s.c_str());
}

int calc_str_width(const char* s)
{
  return (int)display.getTextAdvance(s);
}

struct ClockTextBlock : public TextBlockN<1>
{
  std::string m_hour;
  std::string m_min;
  std::string m_sec;
  std::string m_tsec;
  Prop<bool> m_show_tenth;
   
  int m_width_dbl_digit = 0;
  int m_width_single_digit = 0;
  int m_width_colon = 0;
  int m_width_point = 0;
  int m_width_digit[10] = {0};
  int m_prev_font = -2; // -1 is default font
  int m_prev_size = 0;

   
  ClockTextBlock(IPropHolder* parent, const String& name, bool show = true)
   : TextBlockN(parent, name, LEFT_ALIGN, show)
   , m_show_tenth(this, name + "_show_tenth", true)
  {    
  }

  void recalc_font() {
    int max_width = 0;
    for(int i = 0; i < 9; ++i) {
      int w = calc_str_width(digit_pairs[i]);
      if (w > max_width)
        max_width = w;
        m_width_digit[i] = calc_str_width(digit_single[i]);
    }
    m_width_dbl_digit = max_width;
    m_width_single_digit = max_width / 2;
    m_width_colon = calc_str_width(":");
    m_width_point = calc_str_width("."); 
  }

  void print_pair(int x, const std::string& s) {
    if (s.empty())
      return; // shouldn't happen
    if (s.size() == 1)
      print_str_at(x + m_width_single_digit, m_y.get(), s);
    else {
      int first_digit_num = ((int)s[0]) - '0';
      int nx = x + m_width_single_digit - m_width_digit[first_digit_num];
      print_str_at(nx, m_y.get(), s);
    }
      
    
  }

  void draw() {
    if (!m_show.get())
      return;
    begin_draw();
    do_set_font();
    if (m_font_index.get() != m_prev_font || text_size() != m_prev_size) {
      recalc_font();
      m_prev_font = m_font_index.get();
      m_prev_size = text_size();
    }

    int tw = 0; // total width
    if (!m_hour.empty()) {
      tw += m_width_dbl_digit + m_width_colon;
    }
    tw += m_width_dbl_digit + m_width_colon; // min
    tw += m_width_dbl_digit; // sec
    if (m_show_tenth.get())
      tw += m_width_point + m_width_single_digit;

    int x = m_x.get() - (tw / 2);
    int y = m_y.get();
    
    if (!m_hour.empty()) {
      if (m_hour.size() == 1)
        x += m_width_single_digit;
      print_str_at(x, y, m_hour);
      if (m_hour.size() == 1)
        x += m_width_single_digit;
      else
        x += m_width_dbl_digit;
      print_str_at(x, y, ":");
      x += m_width_colon;
    }

    print_pair(x, m_min);
    x += m_width_dbl_digit;
    print_str_at(x, y, ":");
    x += m_width_colon;

    print_pair(x, m_sec);
    x += m_width_dbl_digit;

    if (m_show_tenth.get()) {
      print_str_at(x, y, ".");
      x += m_width_point;
      print_str_at(x, y, m_tsec);
    }
    end_draw();
  }

};


// clock time h:mm[:ss] where every field has a fixed width slot, as wide as the widest value
// it can show in the current font, and the value is right aligned in it.
// this way the colons don't move when the digits change, also with non-monospace fonts
struct TimeTextBlock : public TextBlock
{
  int m_hour = 0;
  int m_min = 0;
  int m_sec = 0;
  bool m_show_sec = true;

  int m_hour_w = 0;  // slot width of 0-23
  int m_pair_w = 0;  // slot width of 00-59
  int m_colon_w = 0;
  int m_prev_font = -2; // -1 is default font
  int m_prev_size = 0;

  TimeTextBlock(IPropHolder* parent, const String& name, bool show = true)
    : TextBlock(parent, name, LEFT_ALIGN, show)
  {}

  void set_time(int h, int m, int s, bool show_sec) {
    m_hour = h;
    m_min = m;
    m_sec = s;
    m_show_sec = show_sec;
  }

  static void format(char* buf, int v, bool zero_pad) {
    snprintf(buf, 3, zero_pad ? "%02d" : "%d", v);
  }

  void recalc_font() {
    char buf[3];
    m_hour_w = 0;
    for(int i = 0; i < 24; ++i) {
      format(buf, i, false);
      m_hour_w = max(m_hour_w, calc_str_width(buf));
    }
    m_pair_w = 0;
    for(int i = 0; i < 60; ++i) {
      format(buf, i, true);
      m_pair_w = max(m_pair_w, calc_str_width(buf));
    }
    m_colon_w = calc_str_width(":");
  }

  // prints v right aligned in the slot that starts at x, returns the x after the slot
  int print_slot(int x, int slot_w, int v, bool zero_pad) {
    char buf[3];
    format(buf, v, zero_pad);
    print_str_at(x + slot_w - calc_str_width(buf), m_y.get(), buf);
    return x + slot_w;
  }

  void draw() {
    if (!m_show.get())
      return;
    begin_draw();
    do_set_font();
    if (m_font_index.get() != m_prev_font || text_size() != m_prev_size) {
      recalc_font();
      m_prev_font = m_font_index.get();
      m_prev_size = text_size();
    }
    int x = m_x.get();
    x = print_slot(x, m_hour_w, m_hour, false);
    print_str_at(x, m_y.get(), ":");
    x += m_colon_w;
    x = print_slot(x, m_pair_w, m_min, true);
    if (m_show_sec) {
      print_str_at(x, m_y.get(), ":");
      x += m_colon_w;
      print_slot(x, m_pair_w, m_sec, true);
    }
    end_draw();
  }
};




const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * short_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"};

enum FieldKind {
  FIELD_HOUR, FIELD_MIN, FIELD_SEC, FIELD_TENTH, FIELD_WEEKDAY, FIELD_DAY, FIELD_MONTH, FIELD_YEAR
};

// a single part of a date / time. numbers are right aligned in a slot as wide as the widest value
// the field can have in the current font, so that it doesn't move when the value changes. x is the left of the slot.
// same as FieldTextBlock in the web-ui
struct FieldTextBlock : public TextBlock
{
  FieldKind m_kind;
  bool m_hide_zero;  // timer / stop-watch hours aren't shown when 0
  int m_value = 0;
  int m_slot_w = 0;
  int m_prev_font = -2; // -1 is default font
  int m_prev_size = 0;

  FieldTextBlock(IPropHolder* parent, const String& name, FieldKind kind, bool show, bool hide_zero = false)
    : TextBlock(parent, name, LEFT_ALIGN, show), m_kind(kind), m_hide_zero(hide_zero)
  {}

  void set_value(int v) {
    m_value = v;
  }

  void format(char* buf, size_t n, int v) const {
    switch (m_kind) {
      case FIELD_MIN: case FIELD_SEC: snprintf(buf, n, "%02d", v); break;
      case FIELD_WEEKDAY: snprintf(buf, n, "%s", short_days[((v % 7) + 7) % 7]); break;
      default: snprintf(buf, n, "%d", v); break;
    }
  }

  // the values the slot needs to fit. the weekday is left aligned and the year always has 4 digits
  bool range(int& lo, int& hi) const {
    switch (m_kind) {
      case FIELD_HOUR: lo = 0; hi = 23; return true;
      case FIELD_MIN: case FIELD_SEC: lo = 0; hi = 59; return true;
      case FIELD_TENTH: lo = 0; hi = 9; return true;
      case FIELD_DAY: lo = 1; hi = 31; return true;
      case FIELD_MONTH: lo = 1; hi = 12; return true;
      default: return false;
    }
  }

  void recalc_font() {
    m_slot_w = 0;
    int lo, hi;
    if (!range(lo, hi))
      return;
    char buf[8];
    for (int v = lo; v <= hi; ++v) {
      format(buf, sizeof(buf), v);
      m_slot_w = max(m_slot_w, calc_str_width(buf));
    }
  }

  void draw() {
    if (!m_show.get() || (m_hide_zero && m_value == 0))
      return;
    begin_draw();
    do_set_font();
    if (m_font_index.get() != m_prev_font || text_size() != m_prev_size) {
      recalc_font();
      m_prev_font = m_font_index.get();
      m_prev_size = text_size();
    }
    char buf[8];
    format(buf, sizeof(buf), m_value);
    int x = m_x.get();
    if (m_slot_w > 0)
      x += m_slot_w - calc_str_width(buf);
    print_str_at(x, m_y.get(), buf);
    end_draw();
  }
};


// the texts of a timer / stop-watch for one orientation:
// the combined h:mm:ss.t line and the separate fields
struct TimeLayout : public PropHolder<5>
{
  ClockTextBlock m_text;
  FieldTextBlock m_hour;
  FieldTextBlock m_min;
  FieldTextBlock m_sec;
  FieldTextBlock m_tenth;

  TimeLayout(IPropHolder* parent, const String& name, bool portrait)
    : PropHolder(parent)
    , m_text(this, name + "_t1", !portrait)
    , m_hour(this, name + "_fh", FIELD_HOUR, portrait, true)
    , m_min(this, name + "_fm", FIELD_MIN, portrait)
    , m_sec(this, name + "_fs", FIELD_SEC, portrait)
    , m_tenth(this, name + "_ft", FIELD_TENTH, portrait)
  {
    if (portrait) {
      m_hour.init(20, 10, true);
      m_min.init(20, 36, true);
      m_sec.init(20, 62, true);
      m_tenth.init(29, 88);
    }
  }

  void set_time(time_t msec) {
    format_time_sp(msec, m_text.m_hour, m_text.m_min, m_text.m_sec, m_text.m_tsec);
    int h, m, s, t;
    separate_time(msec, h, m, s, t);
    m_hour.set_value(h);
    m_min.set_value(m);
    m_sec.set_value(s);
    m_tenth.set_value(t);
  }

  void draw() {
    m_text.draw();
    m_hour.draw();
    m_min.draw();
    m_sec.draw();
    m_tenth.draw();
  }
};


// ---- background effects of the clock faces ----

enum SpecialEffect {
  FX_NONE = 0,
  FX_BORDER = 1,  // a border that fades to black towards the middle, optionally pulsing
  FX_SLIDE = 2,   // a band that fades to black on its sides and sweeps from side to side
};

// the brightness of an effect (0-1) as the leds should light it to look like the web-ui's preview.
// a screen shows dim colors much darker than they are (its gamma), leds light them as they are: the faint end of
// a fade, that is almost black on the screen, was clearly lit on the panel and stopped at once instead of fading
// to black. only for the effects, the panel's gamma pref is for everything
const float EFFECT_GAMMA = 2.2f;
inline float led_level(float a)
{
    return a <= 0 ? 0 : powf(a, EFFECT_GAMMA);
}

// the background effect, over the clock's background and under the texts. t in seconds.
// same as draw_effect() in the web-ui, but the leds get led_level() of its brightness
void draw_effect(int fx, uint16_t color, int fx_size, int fx_speed, bool pulse, float t)
{
    int w = display.width(), h = display.height();
    uint8_t r, g, b;
    color888(color, r, g, b);
    int size = constrain(fx_size, 1, 64);
    float speed = constrain(fx_speed, 1, 10);
    if (fx == FX_BORDER) {
        float k = 1.0f;
        if (pulse)
            k = 0.6f + 0.4f * sinf(2 * PI * t * speed / 6.0f);
        // the color of each ring, d pixels from the edge
        uint8_t ring[64][3];
        for (int d = 0; d < size; ++d) {
            float a = 1.0f - (float)d / size;
            a = led_level(a * a * k);
            ring[d][0] = r * a;
            ring[d][1] = g * a;
            ring[d][2] = b * a;
        }
        for (int y = 0; y < h; ++y) {
            bool edge_row = y < size || y >= h - size;
            for (int x = 0; x < w; ++x) {
                if (!edge_row && x == size) // skip the middle of the row
                    x = w - size;
                int d = min(min(x, y), min(w - 1 - x, h - 1 - y));
                if (d >= size)
                    continue;
                display.drawPixel(x, y, ring[d][0], ring[d][1], ring[d][2]);
            }
        }
    }
    else if (fx == FX_SLIDE) {
        float period = 12.0f / speed;
        float ph = fmodf(t, period) / period;
        float tri = ph < 0.5f ? ph * 2 : 2 - ph * 2;
        float half = size / 2.0f;
        float cx = -half + tri * (w + size);
        for (int x = max(0, (int)(cx - half)); x <= min(w - 1, (int)(cx + half) + 1); ++x) {
            float a = 1.0f - fabsf(x + 0.5f - cx) / half;
            if (a <= 0)
                continue;
            a = led_level(a * a);
            uint8_t cr = r * a, cg = g * a, cb = b * a;
            for (int y = 0; y < h; ++y)
                display.drawPixel(x, y, cr, cg, cb);
        }
    }
}

// the clock for one orientation: the combined time and date lines, the separate fields and the background
struct ClockLayout : public PropHolder<12>, public BasePanel
{
    TimeTextBlock m_time_text;  // h:mm[:ss]
    TextBlock m_date_text;      // [day] d/m/yyyy
    FieldTextBlock m_hour;
    FieldTextBlock m_min;
    FieldTextBlock m_sec;
    FieldTextBlock m_weekday;
    FieldTextBlock m_day;
    FieldTextBlock m_month;
    FieldTextBlock m_year;
    Prop<uint16_t> m_back_color;
    Prop<bool> m_show_sec;  // not used any more (the time is always h:mm:ss, ClockFace has h:mm), kept for the blob
    Prop<bool> m_show_day;  // of the combined date

    ClockLayout(IPropHolder* parent, const String& name, bool portrait)
      : PropHolder(parent)
      , m_time_text(this, name + "_t1", !portrait)
      , m_date_text(this, name + "_t2", LEFT_ALIGN, !portrait)
      , m_hour(this, name + "_fh", FIELD_HOUR, portrait)
      , m_min(this, name + "_fm", FIELD_MIN, portrait)
      , m_sec(this, name + "_fs", FIELD_SEC, portrait)
      , m_weekday(this, name + "_fw", FIELD_WEEKDAY, portrait)
      , m_day(this, name + "_fd", FIELD_DAY, portrait)
      , m_month(this, name + "_fo", FIELD_MONTH, portrait)
      , m_year(this, name + "_fy", FIELD_YEAR, portrait)
      , m_back_color(this, name + "_back_col", 0)
      , m_show_sec(this, name + "_show_sec", true)
      , m_show_day(this, name + "_show_day", false)
    {
        if (portrait) {
            m_hour.init(20, 8, true);
            m_min.init(20, 30, true);
            m_sec.init(26, 52);
            m_weekday.init(23, 68);
            m_day.init(26, 80);
            m_month.init(26, 92);
            m_year.init(20, 104);
        }
        else {
            m_date_text.m_y.set(32);
        }
    }

    void set_time(time_t local)
    {
        String cur_date;
        if (m_show_day.get()) {
            cur_date += short_days[weekday(local)-1];
            cur_date += " ";
        }
        cur_date += day(local);
        cur_date += "/";
        cur_date += month(local);
        cur_date += "/";
        cur_date += year(local);
        m_date_text.set(cur_date);
        m_time_text.set_time(hour(local), minute(local), second(local), true);

        m_hour.set_value(hour(local));
        m_min.set_value(minute(local));
        m_sec.set_value(second(local));
        m_weekday.set_value(weekday(local) - 1);
        m_day.set_value(day(local));
        m_month.set_value(month(local));
        m_year.set_value(year(local));
    }

    void draw_back()
    {
        color888(m_back_color.get(), m_r, m_g, m_b);
        drawBack();
    }

    void draw_texts(time_t local)
    {
        // the text blocks set the drawer they need.
        // each one by its own type, draw() isn't virtual
        set_time(local);
        m_time_text.draw();
        m_date_text.draw();
        m_hour.draw();
        m_min.draw();
        m_sec.draw();
        m_weekday.draw();
        m_day.draw();
        m_month.draw();
        m_year.draw();
    }
};

// ---- clock faces and the periods of the day, same as in the web-ui (clock.js) ----
// a face is everything about how the clock looks: both layouts and the background effect, with a name.
// a panel without saved faces (new) shows the first face, with the defaults, all day.
// the day is divided into periods, each shows one of the faces. a period starts at its start minute and lasts
// until the next one starts, the last one goes on over midnight until the first one.
// the faces are in CLOCK_FACES slots, each saved as one blob ("lf0".."lf7"), the periods in DAY_PERIODS slots
// saved together in the blob "per": a key per prop would be too many for Preferences

const int CLOCK_FACES = 8;
const int DAY_PERIODS = 12;
const int DAY_MINUTES = 24 * 60;

// how a face shows the clock
enum FaceMode {
    FACE_NORMAL = 0,  // the layouts and the effect
    FACE_OFF = 1,     // nothing lit, the panel is dark (also no mode icon)
};

// new props of a face (or a period) have to be added at the end, the blobs are in this order
struct ClockFace : public BlobHolder<13>
{
    Prop<bool> m_on;
    TextProp m_name;
    ClockLayout m_land;
    ClockLayout m_port;
    Prop<int16_t> m_fx;
    Prop<uint16_t> m_fx_color;
    Prop<int16_t> m_fx_size;  // thickness in pixels
    Prop<int16_t> m_fx_speed; // 1-10
    Prop<bool> m_fx_pulse;    // the border pulses
    Prop<bool> m_wake;        // when its period starts the clock is shown, also when the display is off or in a game
    Prop<int16_t> m_mode;     // FaceMode
    // the time without seconds, of the landscape and the portrait layout. here and not in ClockLayout so that
    // the blobs from before it still load: new props go at the end
    TimeTextBlock m_hhmm_land;
    TimeTextBlock m_hhmm_port;

    ClockFace(IPropHolder* parent, int slot)
      : BlobHolder(parent, String("lf") + slot)
      , m_on(this, m_key + "_on", false)
      , m_name(this, m_key + "_name", (String("Face ") + (slot + 1)).c_str())
      , m_land(this, m_key + "l", false)
      , m_port(this, m_key + "p", true)
      , m_fx(this, m_key + "_fx", FX_NONE)
      , m_fx_color(this, m_key + "_fx_col", 0xF800)
      , m_fx_size(this, m_key + "_fx_size", 8)
      , m_fx_speed(this, m_key + "_fx_speed", 3)
      , m_fx_pulse(this, m_key + "_fx_pulse", true)
      , m_wake(this, m_key + "_wake", false)
      , m_mode(this, m_key + "_mode", FACE_NORMAL)
      , m_hhmm_land(this, m_key + "l_t3", false)
      , m_hhmm_port(this, m_key + "p_t3", false)
    {}

    bool dark() const {
        return m_mode.get() == FACE_OFF;
    }

    bool animated() const {
        return !dark() && (m_fx.get() == FX_SLIDE || (m_fx.get() == FX_BORDER && m_fx_pulse.get()));
    }

    void draw(time_t local, float t) {
        if (dark()) {
            display.clearDisplay();
            return;
        }
        ClockLayout& l = display.is_portrait() ? m_port : m_land;
        l.draw_back();
        if (m_fx.get() != FX_NONE)
            draw_effect(m_fx.get(), m_fx_color.get(), m_fx_size.get(), m_fx_speed.get(), m_fx_pulse.get(), t);
        l.draw_texts(local);
        TimeTextBlock& hhmm = display.is_portrait() ? m_hhmm_port : m_hhmm_land;
        hhmm.set_time(hour(local), minute(local), second(local), false);
        hhmm.draw();
    }
};

struct DayPeriod : public PropHolder<3>
{
    Prop<bool> m_on;
    Prop<int16_t> m_start;  // minute of the day
    Prop<int16_t> m_face;   // the slot of its face

    DayPeriod(IPropHolder* parent, int slot)
      : PropHolder(parent)
      , m_on(this, String("per") + slot + "_on", false)
      , m_start(this, String("per") + slot + "_start", 0)
      , m_face(this, String("per") + slot + "_face", 0)
    {}
};

struct DayPeriods : public BlobHolder<DAY_PERIODS>
{
    DayPeriod* m_periods[DAY_PERIODS];

    DayPeriods(IPropHolder* parent)
      : BlobHolder(parent, "per")
    {
        for (int i = 0; i < DAY_PERIODS; ++i)
            m_periods[i] = new DayPeriod(this, i);
    }
};

// the clock faces and the periods of the day
struct ClockPanel : public PropHolder<1 + CLOCK_FACES + 1>
{
    static const unsigned long SIMULATE_MS = 30000;

    Prop<int16_t> m_tz_offset;
    ClockFace* m_faces[CLOCK_FACES];
    DayPeriods m_day;
    // "Show on panel" in the web-ui: the clock is shown as if it's this minute of the day, -1 is the real time.
    // only for SIMULATE_MS after it was set, the web-ui sends it again while it's on, it ends if the page is closed
    int m_sim_minute = -1;
    unsigned long m_sim_set = 0;

    ClockPanel(NamesIndex* name_index)
      : PropHolder(name_index)
      , m_tz_offset(this, "tz_offset", 3)
      , m_day(this)
    {
        // registered after m_day: the order of the props here doesn't matter, every face and the periods are
        // separate blobs
        for (int i = 0; i < CLOCK_FACES; ++i)
            m_faces[i] = new ClockFace(this, i);
    }

    DayPeriod* period(int slot) { return m_day.m_periods[slot]; }

    time_t local_time(time_t utcTime) const {
        return utcTime + m_tz_offset.get() * 3600;
    }

    // minute of the day, -1 stops
    void simulate(int minute) {
        m_sim_minute = (minute >= 0 && minute < DAY_MINUTES) ? minute : -1;
        m_sim_set = millis();
    }
    bool simulating() {
        if (m_sim_minute >= 0 && millis() - m_sim_set >= SIMULATE_MS)
            m_sim_minute = -1;
        return m_sim_minute >= 0;
    }

    // the local time the clock shows: the simulated minute today, with the real seconds
    time_t shown_time(time_t utcTime) {
        time_t local = local_time(utcTime);
        if (!simulating())
            return local;
        return local - (hour(local) * 60 + minute(local)) * 60 + m_sim_minute * 60;
    }

    // the period at a minute of the day: the last one that started, before the first is the last one of the
    // day before. -1 if there are no periods
    int period_at(int minute) {
        int last = -1, cur = -1;
        for (int i = 0; i < DAY_PERIODS; ++i) {
            DayPeriod* p = period(i);
            if (!p->m_on.get())
                continue;
            if (last < 0 || p->m_start.get() > period(last)->m_start.get())
                last = i;
            if (p->m_start.get() <= minute && (cur < 0 || p->m_start.get() > period(cur)->m_start.get()))
                cur = i;
        }
        return cur >= 0 ? cur : last;
    }

    // the period the clock shows now, also the simulated time
    int period_now(time_t utcTime) {
        time_t local = shown_time(utcTime);
        return period_at(hour(local) * 60 + minute(local));
    }

    // the face of a period. a face that was deleted (or no period) is the first face
    ClockFace* face_of(int period_slot) {
        if (period_slot >= 0) {
            int f = period(period_slot)->m_face.get();
            if (f >= 0 && f < CLOCK_FACES && m_faces[f]->m_on.get())
                return m_faces[f];
        }
        for (int i = 0; i < CLOCK_FACES; ++i)
            if (m_faces[i]->m_on.get())
                return m_faces[i];
        return m_faces[0];
    }

    // the period now if its face takes over the display when it starts, else -1. the real time, not simulated
    int wake_period(time_t utcTime) {
        time_t local = local_time(utcTime);
        int p = period_at(hour(local) * 60 + minute(local));
        return (p >= 0 && face_of(p)->m_wake.get()) ? p : -1;
    }

    void draw(time_t utcTime) {
        face_of(period_now(utcTime))->draw(shown_time(utcTime), millis() / 1000.0f);
    }
};


class ClockState : public IScreen
{
public:
    static const unsigned long ANIM_MS = 40; // ~25 frames per second for the moving effects

    ClockPanel m_panel;
    Preferences m_pref;
    unsigned long m_epoch_time = 0;

private:
    int m_drawn_period = -2;
    ClockFace* m_drawn_face = nullptr;
    unsigned long m_last_anim = 0;

public:
    ClockState(NamesIndex* prop_map)
       : m_panel(prop_map)
    {
    }

    void load()
    {
        // begin needs to be called from setup(), not in c'tor
        m_pref.begin("clock", false);
        m_panel.load(m_pref);
    }
    void save()
    {
        m_panel.save(m_pref);
    }
    void toJson(const JsonObject& obj)
    {
        m_panel.toJson(obj);
    }

    // called in loop, returns true if need to draw besides the change of the time:
    // another period or face, or its effect moves
    bool update() {
        int p = m_panel.period_now(m_epoch_time);
        ClockFace* f = m_panel.face_of(p);
        if (p != m_drawn_period || f != m_drawn_face)
            return true;
        if (f->animated() && millis() - m_last_anim >= ANIM_MS)
            return true;
        return false;
    }

    // the face now shows nothing, also no mode icon
    bool dark() {
        return m_panel.face_of(m_panel.period_now(m_epoch_time))->dark();
    }

    void draw() override
    {
        m_drawn_period = m_panel.period_now(m_epoch_time);
        m_drawn_face = m_panel.face_of(m_drawn_period);
        m_last_anim = millis();
        m_panel.draw(m_epoch_time);
    }
};
