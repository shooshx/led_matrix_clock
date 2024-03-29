#pragma once
#include "Prop.h"

struct TimerPanel : public PropHolder<6>
{
  ClockTextBlock m_timer_text;
  Prop<uint16_t> m_hour;
  Prop<uint16_t> m_min;
  Prop<uint16_t> m_sec;

  Prop<int16_t> m_snd_vol;
  Prop<int16_t> m_snd_file_num;

  bool m_running = false;
  int m_dest_time_msec = 0;  // counting to destination time
  int m_cur_diff_msec = 0; // current draw
  int m_last_draw_diff_msec = 0; // last draw we did
  bool m_force_draw = false;

  TimerPanel(const String& name, NamesIndex* name_index)
    : PropHolder(name_index)
    , m_timer_text(this, name + "_t1")
    , m_hour(this, name + "_hours", 0)
    , m_min(this, name + "_min", 30)
    , m_sec(this, name + "_sec", 0)
    , m_snd_vol(this, name + "_snd_vol", 15)
    , m_snd_file_num(this, name + "_snd_file_num", 1)
  {
    m_force_draw = true;  // set initial text
  }
  

  int read_input() {
    return ((m_hour.get() * 60 + m_min.get()) * 60 + m_sec.get()) * 1000;
  }
  
  void change_input(int d_min) {
    if (m_running)
        return;
    set_time();    
    m_cur_diff_msec += d_min * 60 * 1000;
    int h, m, s, ms;
    separate_time(m_cur_diff_msec, h, m, s, ms);
    m_hour.set(h);
    m_min.set(m);
    m_sec.set(s);
  }

  void toggle_run(int v) {
    if (v == 2) // 2 means flip current state
      v = m_running ? 0 : 1;
    int nowv = m_running ? 1 : 0;
    if (v == nowv)
      return;
    if (v == 1) {
      if (m_cur_diff_msec == 0) // timer ran out
        m_cur_diff_msec = read_input();
      m_dest_time_msec = millis() + m_cur_diff_msec;
      m_running = true;
    }
    else {
      m_running = false;
    }
  }

  // called when input is changed
  void set_time() {
    m_cur_diff_msec = read_input();
  }

  void child_changed() override {
    set_time();
    m_force_draw = true;
    //Serial.printf("Timer: child_changed %d\n", m_cur_diff_msec);
  }

  // called in loop to update the text
  // return true if we need to draw
  bool update_time()
  {
    if (m_running) {
      auto now = millis();
      m_cur_diff_msec = m_dest_time_msec - now;
      if (m_cur_diff_msec < 0) {
        m_cur_diff_msec = 0;
        m_running = false;
      }
    }
    if (m_cur_diff_msec == m_last_draw_diff_msec && !m_force_draw)
      return false;
    //Serial.printf("Timer: update_time %d  force=%d\n", m_cur_diff_msec, (int)m_force_draw);   
    m_force_draw = false;

    
    //auto t = format_time(m_cur_diff_msec);
    //Serial.printf("Timer: %s\n", t.c_str());
    //m_timer_text.set(t);

    format_time_sp(m_cur_diff_msec, m_timer_text.m_hour, m_timer_text.m_min, m_timer_text.m_sec, m_timer_text.m_tsec);
    
    m_last_draw_diff_msec = m_cur_diff_msec;
    return true;
  }

  void draw() {
    //Serial.printf("timer clear\n");
    display.clearDisplay();
    //Serial.printf("timer text\n");
    m_timer_text.draw();
  }
  
};


class TimerState : public IScreen
{
public:
    TimerPanel m_panel;
    Preferences m_pref;

public:
    TimerState(NamesIndex* prop_map)
       : m_panel("tm", prop_map)
    {
    }
    
    void load()
    {
        // begin needs to be called from setup(), not in c'tor
        m_pref.begin("timer", false);
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
        m_panel.draw();
    }

    void toggle_run(int v) override
    {
        //Serial.printf("timer toggle %d\n", v);
        m_panel.toggle_run(v);
    }
    void change(int v) override
    {
        //Serial.printf("timer change %d\n", v);
        m_panel.change_input(v);
        save();
    }
};
