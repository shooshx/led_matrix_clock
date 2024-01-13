#pragma once
#include <vector>
#include <functional>

enum BtnState {
  S_UNKNOWN,
  IS_UP,
  DEBOUNCING_TO_DOWN,
  IS_DOWN,
  DEBOUNCING_TO_UP
};

const char* state_names[] = { "UNKNOWN", "UP", "DEBOUNCE_TO_DOWN", "DOWN", "DEBOUNCE_TO_UP" }; 

struct Button {
  Button(int _pin, std::function<void(int)> _click_cb , std::function<void(int)> _long_press_cb)
    : pin(_pin), click_cb(_click_cb), long_press_cb(_long_press_cb)
  {}


  int pin = 0;
  BtnState state = S_UNKNOWN;
  unsigned long debounce_start = 0; // for debounce
  unsigned long down_start = 0;
  int count_long_calls = 0;
  unsigned long last_long_call = 0;
  std::function<void(int)> click_cb;
  std::function<void(int)> long_press_cb;
};

#define DEBOUND_TIME (50)
#define LONG_PRESS (750)

class Buttons {
public:
  void setup(const std::vector<Button> btns) {
    m_btns = btns;
    for(auto& btn : m_btns) {
      pinMode(btn.pin, INPUT_PULLUP);
      btn.state = IS_UP;
    }
  }


  BtnState state_change(Button& btn, bool pressed, unsigned long now)
  {
      if (btn.state == IS_UP) {
        if (pressed) {
          btn.debounce_start = now;
          return DEBOUNCING_TO_DOWN;
        }
        return S_UNKNOWN;
      }
      if (btn.state == IS_DOWN) {
        if (!pressed) {
          btn.debounce_start = now;
          return DEBOUNCING_TO_UP;
        }
        return S_UNKNOWN;
      }

      if (btn.state == DEBOUNCING_TO_DOWN || btn.state == DEBOUNCING_TO_UP) {
        if (now - btn.debounce_start < DEBOUND_TIME)
          return S_UNKNOWN; // skip if it's in waiting time
        // is it still?
        if (pressed)
          return IS_DOWN;
        else
          return IS_UP; 
      }

      return S_UNKNOWN; // shouldn't get here
  }
  
  void scan() {
    auto now = millis();
    for(auto& btn : m_btns) {        
      // 0 means its' pressed
      int b = digitalRead(btn.pin);
      bool pressed = (b == 0);
      //if (pressed)
      //  Serial.printf("%d pressed\n", btn.pin);
      BtnState new_state = state_change(btn, pressed, now);
      //if (new_state != S_UNKNOWN)
      //  Serial.printf("%d state change %s -> %s\n", btn.pin, state_names[btn.state], state_names[new_state]);

      if (new_state == IS_DOWN) {
        btn.down_start = btn.debounce_start;
      }
      if (new_state == IS_UP) {
        if (btn.down_start != 0 && btn.count_long_calls == 0) { // it's now up and it was down, but not a long call
          btn.click_cb(btn.pin);
        }
        btn.down_start = 0;
        btn.count_long_calls = 0;
        btn.last_long_call = 0;
      }
      
      if (btn.down_start != 0 && now - btn.down_start > LONG_PRESS) {
        btn.long_press_cb(btn.pin);
        btn.count_long_calls++;
        btn.down_start = now;
      }

      if (new_state != S_UNKNOWN) { // no change
        btn.state = new_state;
      }
    }
  }
private:
  std::vector<Button> m_btns; 
};
