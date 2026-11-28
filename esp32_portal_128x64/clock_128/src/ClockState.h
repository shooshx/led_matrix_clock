#pragma once
#include "audio.h"
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


#include "Effects.h"

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

#include "RandomFace.h"

// ---- clock faces and the periods of the day, same as in the web-ui (clock.js) ----
// a face is everything about how the clock looks: both layouts and the background effect, with a name.
// a panel without saved faces (new) shows the first face, with the defaults, all day.
// the day is divided into periods, each shows one of the faces. a period starts at its start minute and lasts
// until the next one starts, the last one goes on over midnight until the first one.
// the faces are in CLOCK_FACES slots, each saved as one blob ("lf0".."lf7"), the periods in DAY_PERIODS slots
// saved together in the blob "per": a key per prop would be too many for Preferences

const int CLOCK_FACES = 8;
const int DAY_PERIODS = 12;  // of each day type
const int DAY_TYPES = 4;
const int DAY_MINUTES = 24 * 60;

// how a face shows the clock
enum FaceMode {
    FACE_NORMAL = 0,  // the layouts and the effect
    FACE_OFF = 1,     // nothing lit, the panel is dark (also no mode icon)
    FACE_RANDOM = 2,  // every aspect by chance, another one every m_rnd_int seconds (RandomFace.h)
};

const int SOUND_FILES_MAX = 160;  // the names of a face's sounds, comma separated

// new props of a face (or a period) have to be added at the end, the blobs are in this order
struct ClockFace : public BlobHolder<22>
{
    Prop<bool> m_on;
    TextProp m_name;
    ClockLayout m_land;
    ClockLayout m_port;
    Prop<int16_t> m_fx;
    Prop<uint16_t> m_fx_color;
    Prop<int16_t> m_fx_size;  // 1-64, what it is depends on the effect
    Prop<int16_t> m_fx_speed; // 1-10
    Prop<bool> m_fx_pulse;    // brightens and dims, Border, Slide and Aurora
    Prop<bool> m_wake;        // not used any more (every period start shows the clock, not in a game), kept for the blob
    Prop<int16_t> m_mode;     // FaceMode
    // the time without seconds, of the landscape and the portrait layout. here and not in ClockLayout so that
    // the blobs from before it still load: new props go at the end
    TimeTextBlock m_hhmm_land;
    TimeTextBlock m_hhmm_port;
    Prop<int32_t> m_rnd_int;  // Random: seconds between faces
    Prop<int16_t> m_fx_angle; // degrees, the direction Slide moves in
    // sounds while it's on: every m_snd_every seconds from the start of its period, for m_snd_total seconds
    // (not past the period's end), a random one of m_snd_files, the names of /sounds files separated by ','
    Prop<bool> m_snd_on;
    TextPropN<SOUND_FILES_MAX> m_snd_files;
    Prop<int32_t> m_snd_every;
    Prop<int32_t> m_snd_total;
    Prop<int16_t> m_snd_volume;  // percent: when its period starts (with m_snd_on) the panel's volume is set to it
    // the color of the clock mode icon (bottom left) in landscape and portrait. its names are the layouts' ones,
    // in the web-ui it's an item of the layout (only its color)
    Prop<uint16_t> m_icon_land;
    Prop<uint16_t> m_icon_port;

    static const uint16_t ICON_COLOR_DEF = 0x8410;  // gray, MODE_ICON_COLOR
    uint16_t icon_color() const {
        return (display.is_portrait() ? m_icon_port : m_icon_land).get();
    }

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
      , m_rnd_int(this, m_key + "_rnd_int", RND_INTERVAL_DEF)
      , m_fx_angle(this, m_key + "_fx_angle", 0)
      , m_snd_on(this, m_key + "_snd_on", false)
      , m_snd_files(this, m_key + "_snd_files", "")
      , m_snd_every(this, m_key + "_snd_every", 60)
      , m_snd_total(this, m_key + "_snd_total", 600)
      , m_snd_volume(this, m_key + "_snd_vol", 30)
      , m_icon_land(this, m_key + "l_icon_col", ICON_COLOR_DEF)
      , m_icon_port(this, m_key + "p_icon_col", ICON_COLOR_DEF)
    {}

    bool dark() const {
        return m_mode.get() == FACE_OFF;
    }

    bool random() const {
        return m_mode.get() == FACE_RANDOM;
    }
    // the milliseconds a random face is shown
    unsigned long random_ms() const {
        return (unsigned long)constrain(m_rnd_int.get(), RND_INTERVAL_MIN, RND_INTERVAL_MAX) * 1000UL;
    }

    // the effect moves. not of a random face, that's ClockPanel::m_rnd_fx
    bool animated() const {
        return !dark() && !random() && fx_animated(m_fx.get(), m_fx_pulse.get());
    }

    void draw(time_t local, float t) {
        if (dark()) {
            display.clearDisplay();
            return;
        }
        ClockLayout& l = display.is_portrait() ? m_port : m_land;
        l.draw_back();
        if (m_fx.get() != FX_NONE)
            draw_effect(m_fx.get(), m_fx_color.get(), m_fx_size.get(), m_fx_speed.get(), m_fx_pulse.get(),
                        m_fx_angle.get(), t);
        l.draw_texts(local);
        TimeTextBlock& hhmm = display.is_portrait() ? m_hhmm_port : m_hhmm_land;
        hhmm.set_time(hour(local), minute(local), second(local), false);
        hhmm.draw();
    }
};

// a period of a day type. its names: the prefix ("per" for the first day type, "d1per", "d2per"...) and the
// slot. the first slot is on until it's changed, a new day type is one period over the whole day
struct DayPeriod : public PropHolder<3>
{
    Prop<bool> m_on;
    Prop<int16_t> m_start;  // minute of the day
    Prop<int16_t> m_face;   // the slot of its face

    DayPeriod(IPropHolder* parent, const String& prefix, int slot)
      : PropHolder(parent)
      , m_on(this, prefix + slot + "_on", slot == 0)
      , m_start(this, prefix + slot + "_start", 0)
      , m_face(this, prefix + slot + "_face", 0)
    {}
};

// the periods of a day type, a blob each. the first one's names are the ones from before there were day types
struct DayPeriods : public BlobHolder<DAY_PERIODS>
{
    DayPeriod* m_periods[DAY_PERIODS];

    static String prefix(int type) { return type == 0 ? String("per") : String("d") + type + "per"; }

    DayPeriods(IPropHolder* parent, int type)
      : BlobHolder(parent, prefix(type))
    {
        for (int i = 0; i < DAY_PERIODS; ++i)
            m_periods[i] = new DayPeriod(this, prefix(type), i);
    }
};

// the named types of days ("Weekday", "Off-day"...) and which one each day of the week is. one blob
struct DayTypes : public BlobHolder<DAY_TYPES * 2 + 7>
{
    TextProp* m_name[DAY_TYPES];
    Prop<bool>* m_on[DAY_TYPES];
    Prop<int16_t>* m_week[7];  // Sunday first: the day type of each day of the week

    DayTypes(IPropHolder* parent)
      : BlobHolder(parent, "days")
    {
        static const char* const names[] = { "Weekday", "Off-day", "Day 3", "Day 4" };
        for (int k = 0; k < DAY_TYPES; ++k) {
            m_name[k] = new TextProp(this, String("dt") + k + "_name", names[k]);
            m_on[k] = new Prop<bool>(this, String("dt") + k + "_on", k < 2);
        }
        for (int d = 0; d < 7; ++d)
            m_week[d] = new Prop<int16_t>(this, String("wd") + d, 0);
    }
};

// the clock faces and the periods of the day
// a period that's on now: its id (day type * DAY_PERIODS + slot, -1 if none), when it started and how long it is,
// local times. it can be the last one of the day before, until the first of the day starts
struct PeriodHere {
    int id = -1;
    time_t start = 0;
    long length = 0;  // seconds, until the next one starts
};

struct ClockPanel : public PropHolder<1 + CLOCK_FACES + DAY_TYPES + 1>
{
    static const unsigned long SIMULATE_MS = 30000;

    Prop<int16_t> m_tz_offset;
    ClockFace* m_faces[CLOCK_FACES];
    DayPeriods* m_days[DAY_TYPES];  // the periods of each day type
    DayTypes m_types;
    // "Show on panel" in the web-ui: the clock is shown as if it's this minute of the day, -1 is the real time.
    // only for SIMULATE_MS after it was set, the web-ui sends it again while it's on, it ends if the page is closed
    int m_sim_minute = -1;
    int m_sim_type = 0;  // the day type it shows then
    unsigned long m_sim_set = 0;
    // the random face shown for a face in Random mode, landscape and portrait. made again when another random
    // face is shown, and after its m_rnd_int
    RandomLayout* m_rnd[2];
    RndFx m_rnd_fx[2];
    const ClockFace* m_rnd_face = nullptr;
    unsigned long m_rnd_made = 0;
    RndMeasure m_measure;

    ClockPanel(NamesIndex* name_index)
      : PropHolder(name_index)
      , m_tz_offset(this, "tz_offset", 3)
      , m_types(this)
    {
        // the order of the props here doesn't matter, every face, the periods of each day type and the day
        // types are separate blobs
        for (int k = 0; k < DAY_TYPES; ++k)
            m_days[k] = new DayPeriods(this, k);
        for (int i = 0; i < CLOCK_FACES; ++i)
            m_faces[i] = new ClockFace(this, i);
        for (int i = 0; i < 2; ++i)
            m_rnd[i] = new RandomLayout();
    }

    // the random face for f should be made again: it's another face, or its time is up
    bool random_due(const ClockFace* f) const {
        return m_rnd_face != f || millis() - m_rnd_made >= f->random_ms();
    }
    // a new random face for f, for both orientations
    void make_random(const ClockFace* f) {
        unsigned long start = millis();
        // the panel's long and short side, whatever its rotation now
        int lng = max(display.width(), display.height()), shrt = min(display.width(), display.height());
        make_random_face(*m_rnd[0], m_rnd_fx[0], m_measure, lng, shrt);
        make_random_face(*m_rnd[1], m_rnd_fx[1], m_measure, shrt, lng);
        m_rnd_face = f;
        m_rnd_made = millis();
        Serial.printf("clock: a random face, made in %lu ms\n", m_rnd_made - start);
    }
    // the effect of the random face shown now
    const RndFx& random_fx() const {
        return m_rnd_fx[display.is_portrait() ? 1 : 0];
    }

    void draw_random(time_t local, float t) {
        RandomLayout& rl = *m_rnd[display.is_portrait() ? 1 : 0];
        const RndFx& fx = random_fx();
        rl.m_l.draw_back();
        if (fx.fx != FX_NONE)
            draw_effect(fx.fx, fx.color, fx.size, fx.speed, fx.pulse, fx.angle, t);
        rl.draw(local);
    }

    // a period by its id: day type * DAY_PERIODS + slot
    DayPeriod* period(int id) { return m_days[id / DAY_PERIODS]->m_periods[id % DAY_PERIODS]; }

    time_t local_time(time_t utcTime) const {
        return utcTime + m_tz_offset.get() * 3600;
    }

    // the day type of a day of the week (0 Sunday). one that isn't there (deleted) is the first one
    int day_type(int wday) {
        int k = m_types.m_week[((wday % 7) + 7) % 7]->get();
        return (k >= 0 && k < DAY_TYPES && m_types.m_on[k]->get()) ? k : 0;
    }

    // of the periods of a day type: the one that started last at or before a minute (-1: all of them, the
    // day's last one), or the first one of the day. -1 if it has none
    int last_started(int type, int minute) {
        int best = -1;
        for (int i = 0; i < DAY_PERIODS; ++i) {
            int id = type * DAY_PERIODS + i;
            DayPeriod* p = period(id);
            if (!p->m_on.get() || (minute >= 0 && p->m_start.get() > minute))
                continue;
            if (best < 0 || p->m_start.get() > period(best)->m_start.get())
                best = id;
        }
        return best;
    }
    int first_of(int type) {
        int best = -1;
        for (int i = 0; i < DAY_PERIODS; ++i) {
            int id = type * DAY_PERIODS + i;
            DayPeriod* p = period(id);
            if (p->m_on.get() && (best < 0 || p->m_start.get() < period(best)->m_start.get()))
                best = id;
        }
        return best;
    }

    // the period that's on at a local time: of the day type of its day of the week (or the given one, for the
    // web-ui's preview of a day type). before the day's first period starts, the last one of the day before
    // goes on. it ends when the next one starts, or the first one of the next day
    PeriodHere period_here(time_t local, int force_type = -1) {
        PeriodHere r;
        int wd = weekday(local) - 1;  // 0 Sunday
        int minute_now = hour(local) * 60 + minute(local);
        time_t day0 = local - (minute_now * 60 + second(local));  // midnight
        auto type_of = [&](int d) { return force_type >= 0 ? force_type : day_type(d); };
        int start_wd = wd;
        int id = last_started(type_of(wd), minute_now);
        if (id < 0) {  // the day before's last one
            start_wd = (wd + 6) % 7;
            day0 -= 86400;
            id = last_started(type_of(start_wd), -1);
        }
        if (id < 0)
            return r;
        int start = period(id)->m_start.get();
        // the end: the next start that day, else the first of the next day
        int type = id / DAY_PERIODS, next = -1;
        for (int i = 0; i < DAY_PERIODS; ++i) {
            DayPeriod* p = period(type * DAY_PERIODS + i);
            if (p->m_on.get() && p->m_start.get() > start && (next < 0 || p->m_start.get() < next))
                next = p->m_start.get();
        }
        long end;
        if (next >= 0)
            end = next * 60L;
        else {
            int f = first_of(type_of((start_wd + 1) % 7));
            end = 86400L + (f >= 0 ? period(f)->m_start.get() : 0) * 60L;
        }
        r.id = id;
        r.start = day0 + start * 60L;
        r.length = max(60L, end - start * 60L);
        return r;
    }

    // minute of the day and the day type to show it with ("Show on panel"), -1 stops
    void simulate(int minute, int type) {
        m_sim_minute = (minute >= 0 && minute < DAY_MINUTES) ? minute : -1;
        m_sim_type = (type >= 0 && type < DAY_TYPES) ? type : 0;
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

    // the period the clock shows now, also the simulated time and day type
    int period_now(time_t utcTime) {
        return period_here(shown_time(utcTime), simulating() ? m_sim_type : -1).id;
    }
    // the period of "Show on panel" while it's on, else -1
    int sim_period(time_t utcTime) {
        return simulating() ? period_here(shown_time(utcTime), m_sim_type).id : -1;
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

    // the period now by the real time, not the simulated one
    PeriodHere real_period(time_t utcTime) {
        return period_here(local_time(utcTime));
    }

    void draw(time_t utcTime) {
        ClockFace* f = face_of(period_now(utcTime));
        if (!f->random()) {
            f->draw(shown_time(utcTime), fx_time());
            return;
        }
        if (random_due(f))
            make_random(f);
        draw_random(shown_time(utcTime), fx_time());
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

    // the sounds of the face: which period (and when it started, the same period another day is another one),
    // the last play of it that was due, and if a sound of it plays now
    int m_snd_period = -1;
    time_t m_snd_start = 0;
    long m_snd_k = -1;
    bool m_snd_playing = false;
    bool m_snd_sim = false;            // the period is "Show on panel"'s, not the real one
    unsigned long m_snd_sim_ms = 0;    // when that started
    // start_volume(): the period it saw last (-2 none yet: no start at boot), when that started, the preview's
    int m_vol_period = -2;
    time_t m_vol_start = 0;
    int m_vol_sim_period = -2;
    static const int SND_LATE_S = 2;  // a play starts only this late, not when the clock shows up in the middle

    void stop_face_sound() {
        if (m_snd_playing && audio_playing_file())
            audio_stop();
        m_snd_playing = false;
    }

    // a random one of the face's sounds that are in the file system, nullptr if none
    static const String* pick_sound(ClockFace* f) {
        std::vector<const String*> have;
        String names = f->m_snd_files.get();
        int pos = 0;
        while (pos <= (int)names.length()) {
            int comma = names.indexOf(',', pos);
            if (comma < 0)
                comma = names.length();
            String name = names.substring(pos, comma);
            name.trim();
            for (const String& s : audio_sounds())
                if (name.length() > 0 && s == name)
                    have.push_back(&s);
            pos = comma + 1;
        }
        if (have.empty())
            return nullptr;
        return have[esp_random() % have.size()];
    }

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
        if (f->random() && m_panel.random_due(f))
            return true;  // another random face
        bool moves = f->random() ? m_panel.random_fx().animated() : f->animated();
        if (moves && millis() - m_last_anim >= ANIM_MS)
            return true;
        return false;
    }

    // called in loop: when a period whose face has sounds starts (by the real time, or "Show on panel" starts
    // showing one), the volume that face wants, else -1. whatever the display shows, not at boot
    int start_volume() {
        int vol = -1;
        // the real time
        if (m_epoch_time >= 1600000000UL) {
            PeriodHere ph = m_panel.real_period(m_epoch_time);
            int p = ph.id;
            time_t started = ph.start;
            if (p != m_vol_period || started != m_vol_start) {
                if (m_vol_period != -2 && p >= 0 && m_panel.face_of(p)->m_snd_on.get())
                    vol = m_panel.face_of(p)->m_snd_volume.get();
                m_vol_period = p;
                m_vol_start = started;
            }
        }
        // the preview
        int sp = m_panel.simulating() ? m_panel.sim_period(m_epoch_time) : -2;
        if (sp != m_vol_sim_period) {
            if (sp >= 0 && m_panel.face_of(sp)->m_snd_on.get())
                vol = m_panel.face_of(sp)->m_snd_volume.get();
            m_vol_sim_period = sp;
        }
        return vol < 0 ? -1 : constrain(vol, 0, 100);
    }

    // called in loop: plays the sounds of the face now. shown: the clock is on the display (not another mode, not
    // off). by the real time: when its period ends, or after its total time, what plays stops.
    // while the web-ui's "Show on panel" simulates a period, its sounds play as if it started when the simulation
    // of it started (only their own total time, a preview has no end)
    void update_sound(bool shown) {
        if (!shown) {
            stop_face_sound();
            m_snd_sim = false;
            m_snd_period = -1;
            return;
        }
        int p;
        long elapsed, total_max;
        if (m_panel.simulating()) {
            p = m_panel.sim_period(m_epoch_time);
            if (!m_snd_sim || p != m_snd_period) {
                // the preview started, or of another period
                stop_face_sound();
                m_snd_sim = true;
                m_snd_period = p;
                m_snd_sim_ms = millis();
                m_snd_k = -1;
            }
            elapsed = (long)((millis() - m_snd_sim_ms) / 1000);
            total_max = 0x7FFFFFFFL;
        }
        else {
            if (m_snd_sim) {  // the preview ended
                stop_face_sound();
                m_snd_sim = false;
                m_snd_period = -1;
            }
            if (m_epoch_time < 1600000000UL) {  // before NTP the time is unknown
                stop_face_sound();
                return;
            }
            PeriodHere ph = m_panel.real_period(m_epoch_time);
            p = ph.id;
            if (p < 0) {
                stop_face_sound();
                return;
            }
            elapsed = (long)(m_panel.local_time(m_epoch_time) - ph.start);
            time_t started = ph.start;
            if (p != m_snd_period || started != m_snd_start) {
                // another period: what plays from the one before doesn't go on into it
                stop_face_sound();
                m_snd_period = p;
                m_snd_start = started;
                m_snd_k = -1;
            }
            total_max = ph.length;
        }
        if (p < 0) {
            stop_face_sound();
            return;
        }
        if (m_snd_playing && !audio_playing_file())
            m_snd_playing = false;  // it ended

        ClockFace* f = m_panel.face_of(p);
        long every = max((int32_t)1, f->m_snd_every.get());
        long total = min((long)f->m_snd_total.get(), total_max);
        if (!f->m_snd_on.get() || elapsed >= total) {
            stop_face_sound();
            return;
        }
        long k = elapsed / every;
        if (k != m_snd_k) {
            m_snd_k = k;
            if (elapsed - k * every <= SND_LATE_S) {
                const String* name = pick_sound(f);
                if (name != nullptr) {
                    Serial.printf("clock: sound %s, %ld s into the period\n", name->c_str(), elapsed);
                    audio_play(name->c_str());
                    m_snd_playing = true;
                }
            }
        }
    }

    // the color of the clock mode icon of the face now
    uint16_t icon_color() {
        return m_panel.face_of(m_panel.period_now(m_epoch_time))->icon_color();
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
