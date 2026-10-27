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
    // colors of the special times, instead of m_color / m_outline_color. -1 is none
    int32_t m_color_override = -1;
    int32_t m_outline_override = -1;

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
        // in a special time its outline color decides: black is no outline, else an outline in that color
        bool outline = (m_outline_override >= 0) ? (m_outline_override != 0) : m_outline.get();
        if (outline) {
            uint8_t r, g, b;
            color888(m_outline_override >= 0 ? (uint16_t)m_outline_override : m_outline_color.get(), r, g, b);
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
        display.setTextColor(m_color_override >= 0 ? (uint16_t)m_color_override : m_color.get());
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


// ---- special times: periods of the day when the clock has other colors and a background effect ----

enum SpecialEffect {
  FX_NONE = 0,
  FX_BORDER = 1,  // a border that fades to black towards the middle, optionally pulsing
  FX_SLIDE = 2,   // a band that fades to black on its sides and sweeps from side to side
};

const int CLOCK_TEXTS = 9; // the texts of a clock layout, in the order of ClockLayout::texts()

// a period of the day and how the clock looks in it. same as SpecialPeriod in the web-ui
struct SpecialPeriod : public PropHolder<5 + 2 * CLOCK_TEXTS + 5>
{
    Prop<bool> m_on;
    Prop<int16_t> m_from;     // minutes of the day
    Prop<int16_t> m_to;       // the end, not included. before m_from goes over midnight
    Prop<int16_t> m_fx;
    Prop<uint16_t> m_fx_color;
    Prop<int16_t> m_fx_size;  // thickness in pixels
    Prop<int16_t> m_fx_speed; // 1-10
    Prop<bool> m_fx_pulse;    // the border pulses
    Prop<uint16_t>* m_colors[CLOCK_TEXTS];
    Prop<uint16_t>* m_outlines[CLOCK_TEXTS];

    SpecialPeriod(IPropHolder* parent, const String& name)
      : PropHolder(parent)
      , m_on(this, name + "_on", false)
      , m_from(this, name + "_from", 19 * 60)
      , m_to(this, name + "_to", 20 * 60)
      , m_fx(this, name + "_fx", FX_BORDER)
      , m_fx_color(this, name + "_fx_col", 0xF800)
      , m_fx_size(this, name + "_fx_size", 8)
      , m_fx_speed(this, name + "_fx_speed", 3)
      , m_fx_pulse(this, name + "_fx_pulse", true)
    {
        for (int i = 0; i < CLOCK_TEXTS; ++i) {
            m_colors[i] = new Prop<uint16_t>(this, name + "_c" + i, 0xFFFF);    // white
            m_outlines[i] = new Prop<uint16_t>(this, name + "_o" + i, 0);  // black: no outline
        }
    }

    bool active_at(int minute) const {
        if (!m_on.get())
            return false;
        int from = m_from.get(), to = m_to.get();
        if (from <= to)
            return minute >= from && minute < to;
        return minute >= from || minute < to; // over midnight
    }

    bool animated() const {
        return m_fx.get() == FX_SLIDE || (m_fx.get() == FX_BORDER && m_fx_pulse.get());
    }

    // the background effect, over the clock's background and under the texts. t in seconds.
    // same as draw_effect() in the web-ui
    void draw_effect(float t) const {
        int w = display.width(), h = display.height();
        uint8_t r, g, b;
        color888(m_fx_color.get(), r, g, b);
        int size = constrain((int)m_fx_size.get(), 1, 64);
        float speed = constrain((int)m_fx_speed.get(), 1, 10);
        if (m_fx.get() == FX_BORDER) {
            float k = 1.0f;
            if (m_fx_pulse.get())
                k = 0.6f + 0.4f * sinf(2 * PI * t * speed / 6.0f);
            for (int y = 0; y < h; ++y) {
                bool edge_row = y < size || y >= h - size;
                for (int x = 0; x < w; ++x) {
                    if (!edge_row && x == size) // skip the middle of the row
                        x = w - size;
                    int d = min(min(x, y), min(w - 1 - x, h - 1 - y));
                    if (d >= size)
                        continue;
                    float a = 1.0f - (float)d / size;
                    a = a * a * k;
                    display.drawPixel(x, y, (uint8_t)(r * a), (uint8_t)(g * a), (uint8_t)(b * a));
                }
            }
        }
        else if (m_fx.get() == FX_SLIDE) {
            float period = 12.0f / speed;
            float ph = fmodf(t, period) / period;
            float tri = ph < 0.5f ? ph * 2 : 2 - ph * 2;
            float half = size / 2.0f;
            float cx = -half + tri * (w + size);
            for (int x = max(0, (int)(cx - half)); x <= min(w - 1, (int)(cx + half) + 1); ++x) {
                float a = 1.0f - fabsf(x + 0.5f - cx) / half;
                if (a <= 0)
                    continue;
                a = a * a;
                uint8_t cr = r * a, cg = g * a, cb = b * a;
                for (int y = 0; y < h; ++y)
                    display.drawPixel(x, y, cr, cg, cb);
            }
        }
    }
};

const int SPECIAL_PERIODS = 4;

struct SpecialTimes : public PropHolder<SPECIAL_PERIODS>
{
    SpecialPeriod* m_periods[SPECIAL_PERIODS];

    SpecialTimes(IPropHolder* parent)
      : PropHolder(parent)
    {
        for (int i = 0; i < SPECIAL_PERIODS; ++i)
            m_periods[i] = new SpecialPeriod(this, String("sp") + i);
    }

    // the period at this minute of the day, -1 if none. the first one wins
    int at(int minute) const {
        for (int i = 0; i < SPECIAL_PERIODS; ++i)
            if (m_periods[i]->active_at(minute))
                return i;
        return -1;
    }
};


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
    Prop<bool> m_show_sec;  // of the combined time
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
        m_time_text.set_time(hour(local), minute(local), second(local), m_show_sec.get());

        m_hour.set_value(hour(local));
        m_min.set_value(minute(local));
        m_sec.set_value(second(local));
        m_weekday.set_value(weekday(local) - 1);
        m_day.set_value(day(local));
        m_month.set_value(month(local));
        m_year.set_value(year(local));
    }

    // in the order of the special colors
    TextBlock* texts(int i) {
        TextBlock* t[CLOCK_TEXTS] = { &m_time_text, &m_date_text, &m_hour, &m_min, &m_sec, &m_weekday, &m_day, &m_month, &m_year };
        return t[i];
    }

    // sp is the special period to show, or null
    void draw(time_t local, const SpecialPeriod* sp, float t)
    {
        color888(m_back_color.get(), m_r, m_g, m_b);
        drawBack();
        if (sp != nullptr)
            sp->draw_effect(t);
        for (int i = 0; i < CLOCK_TEXTS; ++i) {
            texts(i)->m_color_override = sp ? sp->m_colors[i]->get() : -1;
            texts(i)->m_outline_override = sp ? sp->m_outlines[i]->get() : -1;
        }
        // the text blocks set the drawer they need.
        // each one by its own type, draw() isn't virtual so texts(i)->draw() would draw them all as plain text
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

// the clock with a layout for landscape and one for portrait, and the special times
struct ClockPanel : public PropHolder<4>
{
    ClockLayout m_land;
    ClockLayout m_port;
    Prop<int16_t> m_tz_offset;
    SpecialTimes m_special;
    int m_demo = -1;               // a special period shown for a while, from the web-ui
    unsigned long m_demo_until = 0;

    ClockPanel(const String& land_name, const String& port_name, NamesIndex* name_index)
      : PropHolder(name_index)
      , m_land(this, land_name, false)
      , m_port(this, port_name, true)
      , m_tz_offset(this, "tz_offset", 3)
      , m_special(this)
    {
    }

    time_t local_time(time_t utcTime) const {
        return utcTime + m_tz_offset.get() * 3600;
    }

    // the special period shown now, -1 if none
    int special_now(time_t utcTime) {
        if (m_demo >= 0) {
            if ((long)(millis() - m_demo_until) < 0)
                return m_demo;
            m_demo = -1;
        }
        time_t local = local_time(utcTime);
        return m_special.at(hour(local) * 60 + minute(local));
    }

    void demo(int period, unsigned long ms) {
        m_demo = (period >= 0 && period < SPECIAL_PERIODS) ? period : -1;
        m_demo_until = millis() + ms;
    }

    void draw(time_t utcTime)
    {
        int sp = special_now(utcTime);
        (display.is_portrait() ? m_port : m_land).draw(local_time(utcTime), sp >= 0 ? m_special.m_periods[sp] : nullptr, millis() / 1000.0f);
    }
};

class ClockState : public IScreen
{
public:
    static const unsigned long ANIM_MS = 40; // ~25 frames per second for the moving effects
    static const unsigned long DEMO_MS = 30000;

    ClockPanel m_panel;
    Preferences m_pref;
    unsigned long m_epoch_time = 0;

private:
    int m_drawn_special = -2;
    unsigned long m_last_anim = 0;

public:
    ClockState(NamesIndex* prop_map)
       : m_panel("p0", "cp", prop_map)
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

    // shows a special period for a while, -1 stops
    void demo(int period) {
        m_panel.demo(period, DEMO_MS);
    }

    // called in loop, returns true if need to draw besides the change of the time:
    // a special period started or ended, or its effect moves
    bool update() {
        int sp = m_panel.special_now(m_epoch_time);
        if (sp != m_drawn_special)
            return true;
        if (sp >= 0 && m_panel.m_special.m_periods[sp]->animated() && millis() - m_last_anim >= ANIM_MS)
            return true;
        return false;
    }

    void draw() override
    {
        m_drawn_special = m_panel.special_now(m_epoch_time);
        m_last_anim = millis();
        m_panel.draw(m_epoch_time);
    }
};
