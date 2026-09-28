#include <iostream>
#include "../arduino/WString.h"

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
int strSplitStream(String& s, const TF& cb)
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


int main()
{
    {
        String sp[5];
        String in(" AA bbbbbbb   123 ");
        int c = strSplit(in, sp, 5);
        std::cout << c;
    }
    {
        String sp[5];
        String in("   ");
        int c = strSplit(in, sp, 5);
        std::cout << c;
    }
    {
        String sp[5];
        String in("DP 1 18 7 9561");
        int c = strSplit(in, sp, 5);
        std::cout << c;
    }
    {
        String in("DP 1 18 7 9561");
        int c = strSplitStream(in, [](const char* s) {
            std::cout << s << std::endl;
        });
        std::cout << c;
    }
}