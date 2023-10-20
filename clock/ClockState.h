#pragma once

extern PxMATRIX display;

struct TextBlock
{
    int font_index = -1;
    int x = 0;
    int y = 0;
    uint16_t color = 0xffff;

    void draw(const String& text)
    {
        if (font_index < 0 || font_index > sizeof(all_fonts)/sizeof(all_fonts[0]) )
            display.setFont(nullptr);
        else
            display.setFont(all_fonts[font_index].fontPtr);
        display.setTextColor(color);
        display.setCursor(x, y);
        display.print(text);
    }
    
};

const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * short_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"}; 

struct ClockPanel
{
    TextBlock text1;
    TextBlock text2;
    uint16_t back_color = 0;
    bool show_sec = true;
    bool show_day = false;

    String cur_date;
    String cur_time;

    ClockPanel()
    {
        text2.y = 16;
    }

    void time_to_strings(time_t local)
    {
        cur_date = "";  // clear the variables
        cur_time = "";
    
        // now format the Time variables into strings with proper names for month, day etc
        if (show_day) {
            cur_date += short_days[weekday(local)-1];
            cur_date += " ";
        }

        cur_date += day(local);
        cur_date += "/";
        //cur_date += months[month(local)-1];
        cur_date += month(local);
        cur_date += "/";
        cur_date += year(local);
    
        cur_time += hour(local);
        int sec = second(local);
        cur_time += ":";
        int mn = minute(local);
        if(mn < 10)  // add a zero if minute is under 10
          cur_time += "0";
        cur_time += mn;
        if (show_sec) {
            cur_time += ":";  
            if(sec < 10)  // add a zero if minute is under 10
                cur_time += "0";
            cur_time += sec;
        }
    
        //cur_time += " ";
        //cur_time += ampm[isPM(local)];
    }


    void draw(time_t localTime)
    {
        display.clearDisplay();
        time_to_strings(localTime);
        text1.draw(cur_time);
        text2.draw(cur_date);
        
    }
};

class ClockState
{
public:
    ClockPanel panel;

public:
    void draw(time_t localTime)
    {
        panel.draw(localTime);
    }

};
