#pragma once

int strSplit(String& s, String* arr, int arr_len)
{
    int added_count = 0;
    int cur_sz = 0;
    int cur_start = 0;
    for (size_t i = 0; i < s.length(); ++i)
    {
        auto c = s[i];
        if (c == ' ') {
            s[i] = 0;
            if (cur_sz > 0)
                arr[added_count++] = String(s.c_str() + cur_start);
            if (added_count >= arr_len)
                return added_count;
            cur_start = -1;
            cur_sz = 0;
        }
        else {
            if (cur_start == -1)
                cur_start = i;
            ++cur_sz;
        }
    }
    if (cur_sz > 0)
        arr[added_count++] = String(s.c_str() + cur_start);
    return added_count;
}

template<typename TF>
int strSplitStream(String& s, TF& cb)
{
    int added_count = 0;
    int cur_sz = 0;
    int cur_start = 0;
    for (size_t i = 0; i < s.length(); ++i)
    {
        auto c = s[i];
        //Serial.printf("SPLIT char %d - %d\n", i, (int)c);
        if (c == ' ') {
            //Serial.printf("SPLIT space %d - %d, %d\n", i, cur_start, cur_sz);
            s[i] = 0;
            if (cur_sz > 0)
                cb(s.c_str() + cur_start);
            cur_start = -1;
            cur_sz = 0;
        }
        else {
            if (cur_start == -1)
                cur_start = i;
            ++cur_sz;
        }
    }
    if (cur_sz > 0)
        cb(s.c_str() + cur_start);
    return added_count;
}

struct Timer
{
    unsigned long start_time = 0;
    Timer() {
        start_time = micros();
    }
    unsigned long elapsed() {
        return micros() - start_time;
    }
    unsigned long restart() {
        unsigned long t = micros();
        unsigned long r = t - start_time;
        start_time = t;
        return r;
    }
};

String format_time(time_t tmsec)
{
    int64_t d = tmsec;
    int h = trunc(d / (60*60*1000));
    d -= h * (60*60*1000);
    int m = trunc(d / (60*1000));
    d -= m * (60*1000);
    int s = trunc(d / 1000);
    d -= s * 1000;
    int ms = trunc(d / 100);

    String t;
    if (h > 0) {
      t += h;
      t += ":";
      if (m < 10)
        t += "0";
    }
    t += m;
    t += ":";
    if (s < 10)
      t += "0";
    t += s;

    t += ".";
    t += ms;
    return t;
}

const char* digit_pairs[] = {"00", "11", "22", "33", "44", "55", "66", "77", "88", "99" };
const char* digit_single[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const char* digit_zero_single[] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09" };

std::string two_digit_str(int n) {
    int tens = n / 10;
    int ones = n % 10;
    if (tens == 0)
        return std::string(digit_single[ones]);
    std::string s = digit_single[tens];
    s += digit_single[ones];  
    return s;
}

void format_time_sp(time_t tmsec, std::string& shour, std::string& smin, std::string& ssec, std::string& stsec)
{
    int64_t d = tmsec;
    int h = trunc(d / (60*60*1000));
    d -= h * (60*60*1000);
    int m = trunc(d / (60*1000));
    d -= m * (60*1000);
    int s = trunc(d / 1000);
    d -= s * 1000;
    int ms = trunc(d / 100);

    if (h > 0)
      shour = two_digit_str(h);
    else
      shour.clear();
      
    if (h > 0 && m < 10)
      smin = digit_zero_single[m];
    else
      smin = two_digit_str(m);
    
    if (s < 10)
      ssec = digit_zero_single[s];
    else
      ssec = two_digit_str(s);
    
    stsec = two_digit_str(ms);
}
