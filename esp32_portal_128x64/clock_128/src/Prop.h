#pragma once
#include <unordered_map>
#include <vector>
#include "base_utils.h"

class IProp
{
public:
    IProp() {}
    virtual void load(Preferences& pref) = 0;
    virtual bool save(Preferences& pref) = 0;
    virtual void toJson(const JsonObject& obj) = 0;
    virtual bool setInt(int v) {
        Serial.printf("bad setInt in IProp");
        return false;
    }
    // a text pref (TextProp)
    virtual bool setText(const String& v) {
        Serial.printf("bad setText in IProp");
        return false;
    }

    // the values in a blob instead of a Preferences key each, see BlobHolder.
    // in the order the props were made, so new props have to be added at the end of a blob holder
    virtual size_t blob_size() const { return 0; }
    virtual void to_blob(uint8_t*& p) const {}
    // p moves on, values past end aren't read (a blob from before props were added) and keep their defaults
    virtual void from_blob(const uint8_t*& p, const uint8_t* end) {}
    virtual bool is_dirty() const { return false; }
    virtual void clear_dirty() {}

    IProp(const IProp&) = delete;
    IProp& operator=(const IProp&) = delete;
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
    NamesIndex() {
        //Serial.println("NamesIndex ctor");
    }
    int size() const {
        return m_index.size();
    }
    void add(const String& name, IProp* prop) {
        m_index[name] = prop;
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
    PropHolder(IPropHolder* parent)  // for sub-holders
    {
        parent->reg(this, nullptr);
        m_name_index = parent->m_name_index;
    }
    PropHolder(NamesIndex* name_index) // for root-holders
    {
        m_name_index = name_index;
    }
    void reg(IProp* prop, const String* name) override {
        if (m_sz == N) {
            Serial.println("Too many props registering!");
            return;
        }
        m_arr[m_sz++] = prop;
        if (name != nullptr && m_name_index != nullptr)
            m_name_index->add(*name, prop);
    }

    void load(Preferences& pref) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->load(pref);
        child_changed();
    }
    // called on save() whenever one of my children is changed
    virtual void child_changed() {}
    
    bool save(Preferences& pref) override {
        bool any_saved = false;
        for(int i = 0; i < m_sz; ++i)
            if (m_arr[i]->save(pref))
                any_saved = true;
        if (any_saved)
          child_changed();
        return any_saved;
    }
    void toJson(const JsonObject& obj) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->toJson(obj);      
    }

    virtual bool setInt(int v) {
        Serial.printf("bad in PropHolder");
        return false;
    }

    size_t blob_size() const override {
        size_t n = 0;
        for(int i = 0; i < m_sz; ++i)
            n += m_arr[i]->blob_size();
        return n;
    }
    void to_blob(uint8_t*& p) const override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->to_blob(p);
    }
    void from_blob(const uint8_t*& p, const uint8_t* end) override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->from_blob(p, end);
    }
    bool is_dirty() const override {
        for(int i = 0; i < m_sz; ++i)
            if (m_arr[i]->is_dirty())
                return true;
        return false;
    }
    void clear_dirty() override {
        for(int i = 0; i < m_sz; ++i)
            m_arr[i]->clear_dirty();
    }
private:
    IProp* m_arr[N];
    int m_sz = 0;
};

extern bool has_serial;

// Preferences (NVS) keys are limited to 15 chars, longer names get a key made of
// the start of the name and a hash of the whole name
String nvs_key(const String& name)
{
    const size_t MAX_KEY_LEN = 15;
    if (name.length() <= MAX_KEY_LEN)
        return name;
    char buf[MAX_KEY_LEN + 1];
    snprintf(buf, sizeof(buf), "%.6s_%08x", name.c_str(), (unsigned)StringHashOps()(name));
    return String(buf);
}

template<typename T>
class Prop : public IProp
{
public:
    Prop(IPropHolder* holder, const String& name, T init_value) 
      : m_value(init_value), m_name(name), m_key(nvs_key(name))
    {
        holder->reg(this, &m_name);
    }
    
    T m_value;
    String m_name;
    String m_key; // key in Preferences, limited to 15 chars
    bool m_dirty = false;

    bool set(T v) {
        //if (has_serial)
        //Serial.printf("Prop setting %s to %d from %d\n", m_name.c_str(), v, m_value);
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
    bool save(Preferences& pref) override;
    void toJson(const JsonObject& obj) override
    {
        //Serial.printf("toJson %s, %d\n", m_name.c_str(), m_value);
        obj[m_name] = m_value;
    }

    bool setInt(int v) override {
        Serial.printf("bad in Prop %s\n", m_name.c_str());
        return false;
    };

    size_t blob_size() const override { return sizeof(T); }
    void to_blob(uint8_t*& p) const override {
        memcpy(p, &m_value, sizeof(T));
        p += sizeof(T);
    }
    void from_blob(const uint8_t*& p, const uint8_t* end) override {
        if (p + sizeof(T) <= end)
            memcpy(&m_value, p, sizeof(T));
        p += sizeof(T);
        m_dirty = false;
    }
    bool is_dirty() const override { return m_dirty; }
    void clear_dirty() override { m_dirty = false; }
};

template<>
void Prop<bool>::load(Preferences& pref) {
  m_value = pref.getBool(m_key.c_str(), m_value);
  m_dirty = false;
}

template<>
bool Prop<bool>::save(Preferences& pref) {
  if (!m_dirty)
      return false;
  auto r = pref.putBool(m_key.c_str(), m_value);
  if (r == 0)
        Serial.printf("Failed saving b %s, %d\n", m_name.c_str(), m_value);     
  m_dirty = false;
  return true;
}

template<>
bool Prop<bool>::setInt(int v) {
    //Serial.printf("setInt(b) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(v != 0);
}

//-----------------------------------------

template<>
void Prop<uint16_t>::load(Preferences& pref) {
  m_value = pref.getUShort(m_key.c_str(), m_value);
  m_dirty = false;
}

template<>
bool Prop<uint16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return false;
  auto r = pref.putUShort(m_key.c_str(), m_value);
  if (r == 0)
      Serial.printf("Failed saving uint16 %s, %d = %d\n", m_name.c_str(), m_value, r);     
  m_dirty = false;
  return true;
}
template<>
bool Prop<uint16_t>::setInt(int v) {
    //Serial.printf("setInt(16) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(static_cast<uint16_t>(v));
}

// -----------------------------------------------

template<>
void Prop<int16_t>::load(Preferences& pref) {
  m_value = pref.getShort(m_key.c_str(), m_value);
  m_dirty = false;
}

template<>
bool Prop<int16_t>::save(Preferences& pref) {
  if (!m_dirty)
      return false;
  auto r = pref.putShort(m_key.c_str(), m_value);
  if (r == 0)
      Serial.printf("Failed saving int16 %s, %d=%d\n", m_name.c_str(), m_value, r);
  m_dirty = false;
  return true;
}

template<>
bool Prop<int16_t>::setInt(int v) {
    //Serial.printf("setInt(16) %s to %d from %d\n", m_name.c_str(), v, m_value);
    return set(static_cast<int16_t>(v));
}

// -----------------------------------------------

// a short text, like the name of a clock face. sent from the web-ui with "UT name text", in the json as a string
class TextProp : public IProp
{
public:
    static const int MAX_LEN = 16;

    TextProp(IPropHolder* holder, const String& name, const char* init_value)
      : m_name(name), m_key(nvs_key(name))
    {
        m_value[0] = 0;
        set(init_value);
        m_dirty = false;
        holder->reg(this, &m_name);
    }

    char m_value[MAX_LEN + 1];
    String m_name;
    String m_key;
    bool m_dirty = false;

    bool set(const char* v) {
        char buf[MAX_LEN + 1];
        snprintf(buf, sizeof(buf), "%s", v);
        if (strcmp(buf, m_value) == 0)
            return false;
        memcpy(m_value, buf, sizeof(buf));
        m_dirty = true;
        return true;
    }
    const char* get() const {
        return m_value;
    }
    bool setText(const String& v) override {
        return set(v.c_str());
    }
    void load(Preferences& pref) override {
        if (pref.isKey(m_key.c_str()))
            pref.getString(m_key.c_str(), m_value, sizeof(m_value));
        m_dirty = false;
    }
    bool save(Preferences& pref) override {
        if (!m_dirty)
            return false;
        pref.putString(m_key.c_str(), m_value);
        m_dirty = false;
        return true;
    }
    void toJson(const JsonObject& obj) override {
        obj[m_name] = String(m_value);  // copied into the json
    }
    // a fixed size in a blob, with the terminating 0
    size_t blob_size() const override { return sizeof(m_value); }
    void to_blob(uint8_t*& p) const override {
        memcpy(p, m_value, sizeof(m_value));
        p += sizeof(m_value);
    }
    void from_blob(const uint8_t*& p, const uint8_t* end) override {
        if (p + sizeof(m_value) <= end) {
            memcpy(m_value, p, sizeof(m_value));
            m_value[MAX_LEN] = 0;
        }
        p += sizeof(m_value);
        m_dirty = false;
    }
    bool is_dirty() const override { return m_dirty; }
    void clear_dirty() override { m_dirty = false; }
};

// -----------------------------------------------

// props that are saved together as one Preferences blob under the key, instead of a key each.
// for groups with many props, like a clock face: Preferences (NVS) has room for only a few hundred keys.
// the props are still separate for the web-ui (names, toJson, setInt). the blob is the values one after the
// other in the order the props were made, a blob from before props were added at the end still loads
template<int N>
class BlobHolder : public PropHolder<N>
{
public:
    BlobHolder(IPropHolder* parent, const String& key)
      : PropHolder<N>(parent), m_key(key)
    {}

    String m_key;

    void load(Preferences& pref) override {
        size_t n = pref.isKey(m_key.c_str()) ? pref.getBytesLength(m_key.c_str()) : 0;
        if (n > 0) {
            std::vector<uint8_t> buf(n);
            pref.getBytes(m_key.c_str(), buf.data(), n);
            const uint8_t* p = buf.data();
            this->from_blob(p, buf.data() + n);
        }
        this->clear_dirty();
        this->child_changed();
    }

    bool save(Preferences& pref) override {
        if (!this->is_dirty())
            return false;
        std::vector<uint8_t> buf(this->blob_size());
        uint8_t* p = buf.data();
        this->to_blob(p);
        if (pref.putBytes(m_key.c_str(), buf.data(), buf.size()) != buf.size())
            Serial.printf("Failed saving blob %s, %d bytes\n", m_key.c_str(), (int)buf.size());
        this->clear_dirty();
        this->child_changed();
        return true;
    }
};
