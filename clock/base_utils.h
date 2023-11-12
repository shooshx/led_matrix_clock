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
