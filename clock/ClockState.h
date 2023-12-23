#pragma once
#include "Prop.h"
#include "base_utils.h"

extern WrapGFX display;
extern PxMATRIX matrix_drawer;

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
struct TextBlockN : public PropHolder<4 + NParentPropNum>
{
    String m_name;
    String m_text;
    Prop<int16_t> m_font_index;
    Prop<int16_t> m_x;
    Prop<int16_t> m_y;
    Prop<uint16_t> m_color;
    TextAlign m_align = LEFT_ALIGN;

    TextBlockN(IPropHolder* parent, const String& name, TextAlign align = LEFT_ALIGN)
      : PropHolder<4 + NParentPropNum>(parent), m_name(name)
      , m_font_index(this, name + "_font_idx", -1)
      , m_x(this, name + "_x", 0)
      , m_y(this, name + "_y", 0)
      , m_color(this, name + "_color", 0xffff)
      , m_align(align)
    {  
    }

    void set(const String& text) {
        m_text = text;
    }

    void do_set_font() {
        if (m_font_index.get() < 0 || m_font_index.get() > sizeof(all_fonts)/sizeof(all_fonts[0]) )
            display.setFont(nullptr);
        else
            display.setFont(all_fonts[m_font_index.get()].fontPtr);
        display.setTextColor(m_color.get());
    }
     
    void draw()
    {
        Timer t;
        do_set_font();
        
        if (m_align == RIGHT_ALIGN) {
          Rect r;
          int16_t x_after = 0;
          display.getTextBounds(m_text, m_x.get(), m_y.get(), &r.x1, &r.y1, &r.w, &r.h, &x_after);
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
  Rect r;
  int16_t x_after = 0;
  display.getTextBounds(s, 0, 0, &r.x1, &r.y1, &r.w, &r.h, &x_after);
  return (int)x_after;
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

   
  ClockTextBlock(IPropHolder* parent, const String& name)
   : TextBlockN(parent, name, LEFT_ALIGN)
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
    do_set_font();
    if (m_font_index.get() != m_prev_font) {
      recalc_font();
      m_prev_font = m_font_index.get();
    }

    int tw = 0; // total width
    if (!m_hour.empty()) {
      if (m_hour.size() == 1)
        tw += m_F + m_width_colon;
      else
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
    
    
  }

};


const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * short_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"}; 

struct ClockPanel : public PropHolder<6>, public BasePanel
{
    String m_name;
    TextBlock m_time_text;  // time
    TextBlock m_date_text;  // date
    Prop<uint16_t> m_back_color;
    Prop<bool> m_show_sec;
    Prop<bool> m_show_day;
    Prop<int16_t> m_tz_offset;
    
    ClockPanel(const String& name, NamesIndex* name_index)
      : PropHolder(name_index), m_name(name)
      , m_time_text(this, name + "_t1")
      , m_date_text(this, name + "_t2")
      , m_back_color(this, name + "_back_col", 0)
      , m_show_sec(this, name + "_show_sec", true)
      , m_show_day(this, name + "_show_day", false)
      , m_tz_offset(this, "tz_offset", 3)
    {
        //Serial.println("ClockPanel ctor");
        m_date_text.m_y.set(16);
    }

    void time_to_strings(time_t utcTime)
    {
        time_t local = utcTime + m_tz_offset.get() * 3600;
        
        String cur_date;
        String cur_time;
    
        // now format the Time variables into strings with proper names for month, day etc
        if (m_show_day.get()) {
            cur_date += short_days[weekday(local)-1];
            cur_date += " ";
        }

        cur_date += day(local);
        cur_date += "/";
        //cur_date += months[month(local)-1];
        cur_date += month(local);
        cur_date += "/";
        cur_date += year(local);

        int h = hour(local);
        if (h < 10)
            cur_time += "0";
        cur_time += h;
        int sec = second(local);
        cur_time += ":";
        int mn = minute(local);
        if(mn < 10)  // add a zero if minute is under 10
            cur_time += "0";
        cur_time += mn;
        if (m_show_sec.get()) {
            cur_time += ":";  
            if(sec < 10)  // add a zero if minute is under 10
                cur_time += "0";
            cur_time += sec;
        }

        m_time_text.set(cur_time);
        m_date_text.set(cur_date);
    
        //cur_time += " ";
        //cur_time += ampm[isPM(local)];
    }


    void draw(time_t utcTime)
    {
        Timer t;
        //display.clearDisplay();
        drawBack();
        // set the halo drawer only after drawing the back so that the back drawing doesn't go to it
        display.setDrawer(&m_halo_drawer);

        auto tc = t.restart();
        //display.fillScreen(m_back_color.get());
        time_to_strings(utcTime);
        auto ts = t.restart();
        m_time_text.draw();
        m_date_text.draw();
        auto td = t.restart();
        //printf("  clock took %d, %d, %d\n", tc, ts, td);
    }

};

class ClockState : public IScreen
{
public:
    ClockPanel m_panel;
    Preferences m_pref;
    unsigned long m_epoch_time = 0;

public:
    ClockState(NamesIndex* prop_map)
       : m_panel("p0", prop_map)
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
    
    void draw() override
    {
        m_panel.draw(m_epoch_time);
    }



};
