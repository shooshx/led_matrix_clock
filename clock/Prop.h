#pragma once

class IProp
{
public:
    virtual void load(Preferences& pref) = 0;
    virtual void save(Preferences& pref) = 0;
    virtual void toJson(const JsonObject& obj) = 0;
};

class IPropHolder
{
public:
    virtual void reg(IProp* prop) = 0;
};

template<int N>
class PropHolder : public IProp, public IPropHolder
{
public:
    PropHolder(IPropHolder* parent) {
        if (parent != nullptr)
            parent->reg(this);
    }
    void reg(IProp* prop) override {
        if (m_sz == N)
            return;
        m_arr[m_sz++] = prop;
    }
    void load(Preferences& pref) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->load(pref);
    }
    void save(Preferences& pref) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->save(pref);      
    }
    void toJson(const JsonObject& obj) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->toJson(obj);      
    }
private:
    IProp* m_arr[N];
    int m_sz = 0;
};

template<typename T>
class Prop : public IProp
{
public:
    Prop(IPropHolder* holder, const String& name, T init_value) 
      : m_value(init_value), m_name(name)
    {
        holder->reg(this);
    }
    
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
    void load(Preferences& pref) override;
    void save(Preferences& pref) override;
    void toJson(const JsonObject& obj) override
    {
        obj[m_name] = m_value;
    }
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
