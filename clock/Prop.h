#pragma once
#include <unordered_map>

class IProp
{
public:
    virtual void load(Preferences& pref) = 0;
    virtual void save(Preferences& pref) = 0;
    virtual void toJson(const JsonObject& obj) = 0;
    virtual bool setInt(int v) {
        Serial.printf("bad in IProp");
        return false;
    }
};

struct StringHashOps {
  // Fnv1Hash
  size_t operator()(const String& str) const {
      constexpr uint32_t fnv_prime = 16777619u;
      constexpr uint32_t fnv_basis = 2166136261u;
  
      uint32_t hash = fnv_basis;
      for (size_t idx = 0; idx < str.length(); ++idx) {
          hash = hash ^ static_cast<uint32_t>(str.c_str()[idx]);
          hash = hash * fnv_prime;
      }
      return hash;
  }

  bool operator()(const String& lhs, const String& rhs) const {
      return lhs == rhs;
  }
};


class NamesIndex
{
public:
    int size() const {
        return m_index.size();
    }
    void add(const String& name, IProp* prop) {
        m_index[name] = prop;
        Serial.printf("Added prop %s\n", name);
    }
    IProp* get(const String& name) const {
        auto it = m_index.find(name);
        if (it == m_index.end())
            return nullptr;
        return it->second;
    }
private:
    std::unordered_map<String, IProp*, StringHashOps, StringHashOps> m_index;
};

class IPropHolder
{
public:
    virtual void reg(IProp* prop, const String* name) = 0;
public:
    NamesIndex* m_name_index = nullptr;    
};

template<int N>
class PropHolder : public IProp, public IPropHolder
{
public:
    PropHolder(IPropHolder* parent, NamesIndex* name_index = nullptr) 
    {
        if (parent != nullptr) {
            parent->reg(this, nullptr);
        }
        if (name_index != nullptr)
            m_name_index = name_index;
        else if (parent != nullptr)
            m_name_index = parent->m_name_index;
    }
    void reg(IProp* prop, const String* name) override {
        if (m_sz == N)
            return;
        m_arr[m_sz++] = prop;
        if (name != nullptr && m_name_index != nullptr)
            m_name_index->add(*name, prop);
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

    virtual bool setInt(int v) {
        Serial.printf("bad in PropHolder");
        return false;
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
        holder->reg(this, &m_name);
    }
    
    T m_value;
    String m_name;
    bool m_dirty = false;

    bool set(T v) {
        if (m_value == v)
            return false;
        m_value = v;
        m_dirty = true;
        return true;
    }
    T get() const {
        return m_value;
    }
    void load(Preferences& pref) override;
    void save(Preferences& pref) override;
    void toJson(const JsonObject& obj) override
    {
        //Serial.printf("toJson %s, %d\n", m_name.c_str(), m_value);
        obj[m_name] = m_value;
    }

    bool setInt(int v) override {
        Serial.printf("bad in Prop %s\n", m_name.c_str());
        return false;
    };
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
  auto r = pref.putBool(m_name.c_str(), m_value);
  if (r == 0)
        Serial.printf("Failed saving b %s, %d\n", m_name.c_str(), m_value);     
  m_dirty = false;
}

template<>
bool Prop<bool>::setInt(int v) {
    //Serial.printf("setInt(b) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(static_cast<bool>(v));
}

//-----------------------------------------

template<>
void Prop<uint16_t>::load(Preferences& pref) {
  m_value = pref.getUShort(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<uint16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return;
  auto r = pref.putUShort(m_name.c_str(), m_value);
  if (r == 0)
      Serial.printf("Failed saving uint16 %s, %d = %d\n", m_name.c_str(), m_value, r);     
  m_dirty = false;
}
template<>
bool Prop<uint16_t>::setInt(int v) {
    //Serial.printf("setInt(16) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(static_cast<uint16_t>(v));
}

// -----------------------------------------------

template<>
void Prop<int16_t>::load(Preferences& pref) {
  m_value = pref.getShort(m_name.c_str(), m_value);
  m_dirty = false;
}

template<>
void Prop<int16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return;
  auto r = pref.putShort(m_name.c_str(), m_value);
  if (r == 0)
      Serial.printf("Failed saving int16 %s, %d=%d\n", m_name.c_str(), m_value, r);
  m_dirty = false;
}

template<>
bool Prop<int16_t>::setInt(int v) {
    //Serial.printf("setInt(16) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(static_cast<int16_t>(v));
}
