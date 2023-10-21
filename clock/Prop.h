#pragma once

template<typename T>
class Prop
{
public:
    Prop(const String& name, T init_value) : m_value(init_value), m_name(name)
    {}
    
    T m_value;
    String m_name;
    bool m_dirty = false;

    void set(T v) {
        m_value = v;
        m_dirty = true;
    }
    T get() const {
        return m_value;
    }
    void load(Preferences& pref);
    void save(Preferences& pref);
};

template<>
void Prop<bool>::load(Preferences& pref) {
  m_value = pref.getBool(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<bool>::save(Preferences& pref) {
  if (!m_dirty)
      return;
  pref.putBool(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<uint16_t>::load(Preferences& pref) {
  m_value = pref.getUShort(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<uint16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return;
  pref.putUShort(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<int16_t>::load(Preferences& pref) {
  m_value = pref.getShort(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<int16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return;
  pref.putShort(m_name.c_str(), m_value);
  m_dirty = false;
}
