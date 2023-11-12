#pragma once
#include "Prop.h"
#include "base_utils.h"

extern PxMATRIX display;



struct TextBlock : public PropHolder<4>
{
    String m_name;
    Prop<int16_t> m_font_index;
    Prop<int16_t> m_x;
    Prop<int16_t> m_y;
    Prop<uint16_t> m_color;

    TextBlock(IPropHolder* parent, const String& name)
      : PropHolder(parent), m_name(name)
      , m_font_index(this, name + "_font_idx", -1)
      , m_x(this, name + "_x", 0)
      , m_y(this, name + "_y", 0)
      , m_color(this, name + "_color", 0xffff)
    {  
    }
     
    void draw(const String& text)
    {
        Timer t;
        if (m_font_index.get() < 0 || m_font_index.get() > sizeof(all_fonts)/sizeof(all_fonts[0]) )
            display.setFont(nullptr);
        else
            display.setFont(all_fonts[m_font_index.get()].fontPtr);
        display.setTextColor(m_color.get());
        display.setCursor(m_x.get(), m_y.get());
        auto ts = t.restart();
        display.print(text);
        auto tp = t.restart();
        //printf("    text took %d, %d\n", ts, tp);
    }
    
};

const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * short_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"}; 

struct ClockPanel : public PropHolder<5>
{
    String m_name;
    TextBlock m_text1;
    TextBlock m_text2;
    Prop<uint16_t> m_back_color;
    Prop<bool> m_show_sec;
    Prop<bool> m_show_day;

    String m_cur_date;
    String m_cur_time;

    ClockPanel(const String& name, NamesIndex* name_index)
      : PropHolder(name_index), m_name(name)
      , m_text1(this, name + "_t1")
      , m_text2(this, name + "_t2")
      , m_back_color(this, name + "_back_col", 0)
      , m_show_sec(this, name + "_show_sec", true)
      , m_show_day(this, name + "_show_day", false)
    {
        Serial.println("ClockPanel ctor");
        m_text2.m_y.set(16);
    }

    void time_to_strings(time_t local)
    {
        m_cur_date = "";  // clear the variables
        m_cur_time = "";
    
        // now format the Time variables into strings with proper names for month, day etc
        if (m_show_day.get()) {
            m_cur_date += short_days[weekday(local)-1];
            m_cur_date += " ";
        }

        m_cur_date += day(local);
        m_cur_date += "/";
        //cur_date += months[month(local)-1];
        m_cur_date += month(local);
        m_cur_date += "/";
        m_cur_date += year(local);

        int h = hour(local);
        if (h < 10)
            m_cur_time += "0";
        m_cur_time += h;
        int sec = second(local);
        m_cur_time += ":";
        int mn = minute(local);
        if(mn < 10)  // add a zero if minute is under 10
            m_cur_time += "0";
        m_cur_time += mn;
        if (m_show_sec.get()) {
            m_cur_time += ":";  
            if(sec < 10)  // add a zero if minute is under 10
                m_cur_time += "0";
            m_cur_time += sec;
        }
    
        //cur_time += " ";
        //cur_time += ampm[isPM(local)];
    }


    void draw(time_t localTime)
    {
        Timer t;
        display.clearDisplay();
        auto tc = t.restart();
        //display.fillScreen(m_back_color.get());
        time_to_strings(localTime);
        auto ts = t.restart();
        m_text1.draw(m_cur_time);
        m_text2.draw(m_cur_date);
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
    
    void draw(time_t localTime)
    {
        m_panel.draw(localTime);
    }



};
