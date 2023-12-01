#pragma once
#include "Prop.h"
#include "base_utils.h"

extern PxMATRIX display;

struct Rect
{
  int16_t x1 = 0, y1 = 0;
  uint16_t w = 0, h = 0;
};

enum TextAlign {
  LEFT_ALIGN = 0,
  RIGHT_ALIGN = 1
};

struct TextBlock : public PropHolder<4>
{
    String m_name;
    String m_text;
    Prop<int16_t> m_font_index;
    Prop<int16_t> m_x;
    Prop<int16_t> m_y;
    Prop<uint16_t> m_color;
    TextAlign m_align = LEFT_ALIGN;

    TextBlock(IPropHolder* parent, const String& name, TextAlign align = LEFT_ALIGN)
      : PropHolder(parent), m_name(name)
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
     
    void draw()
    {
        Timer t;
        if (m_font_index.get() < 0 || m_font_index.get() > sizeof(all_fonts)/sizeof(all_fonts[0]) )
            display.setFont(nullptr);
        else
            display.setFont(all_fonts[m_font_index.get()].fontPtr);
        display.setTextColor(m_color.get());
        if (m_align == RIGHT_ALIGN) {
          Rect r;
          display.getTextBounds(m_text, m_x.get(), m_y.get(), &r.x1, &r.y1, &r.w, &r.h);
          display.setCursor(m_x.get() - (r.w + r.x1), m_y.get());
        }
        else {
          display.setCursor(m_x.get(), m_y.get());
        }
        
        //auto ts = t.restart();
        display.print(m_text);
        //auto tp = t.restart();

        //auto tb = t.restart();
        //printf("    text took %d, %d, %d\n", ts, tp, tb);
        
    }
    
};


const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * short_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"}; 

struct ClockPanel : public PropHolder<6>
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
        display.clearDisplay();
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

class ClockState
{
public:
    ClockPanel m_panel;
    Preferences m_pref;

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
    
    void draw(time_t utcTime)
    {
        m_panel.draw(utcTime);
    }



};
