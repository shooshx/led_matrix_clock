#pragma once
#include "Prop.h"

struct StopWPanel : public PropHolder<1>
{
  TextBlock m_sw_text;

  bool m_running = false;
  time_t m_start_time_msec = -1; // when running, this is the start time
  time_t m_cur_diff_msec = 0;    // currently shown time
  time_t m_last_draw_diff_msec = 0;
  bool m_force_draw = false;

  StopWPanel(const String& name, NamesIndex* name_index)
    : PropHolder(name_index)
    , m_sw_text(this, name + "_t1", RIGHT_ALIGN)
  {
    m_force_draw = true; // set initial text
  }

  void child_changed() override {
    m_force_draw = true;
  }

  void toggle_run(int v) {
    if (v == 2) // 2 means flip current state
      v = m_running ? 0 : 1;
    int nowv = m_running ? 1 : 0;
    if (v == nowv)
      return;
    if (!m_running) {
      if (m_start_time_msec == -1)
        m_start_time_msec = millis();
      else
        m_start_time_msec = millis() - m_cur_diff_msec; 
    }
    m_running = !m_running;
  }

  void reset() {
    m_cur_diff_msec = 0;
    if (!m_running) {
      m_start_time_msec = -1;
    }
    else {
      m_start_time_msec = millis();
    }
  }

  bool update_time() {
    if (m_running) {
      m_cur_diff_msec = millis() - m_start_time_msec;
    }
    if (m_cur_diff_msec == m_last_draw_diff_msec && !m_force_draw)
      return false;
    m_force_draw = false;

    auto t = format_time(m_cur_diff_msec);
    m_sw_text.set(t);
    m_last_draw_diff_msec = m_cur_diff_msec;
    return true;
  }

  
  void draw() {
    display.clearDisplay();
    m_sw_text.draw();
  }
  
};


class StopWState
{
public:
    StopWPanel m_panel;
    Preferences m_pref;

public:
    StopWState(NamesIndex* prop_map)
       : m_panel("sw", prop_map)
    {
    }
    
    void load()
    {
        // begin needs to be called from setup(), not in c'tor
        m_pref.begin("stopw", false);
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
    
    void draw()
    {
        m_panel.draw();
    }
};
