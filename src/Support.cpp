#include "Support.h"

std::unique_ptr<WCHAR> to_string(int n)
{
    if (n == 0)
    {
        std::unique_ptr<WCHAR> string(new WCHAR[2]);
        _snwprintf_s(string.get(), 2, 2, L"%d\0", n);
        return string;
    }

    int t = n,
        k = 0;
    while (t > 0)
    {
        ++k;
        t /= 10;
    }
    std::unique_ptr<WCHAR> string(new WCHAR[k + 1]);
    _snwprintf_s(string.get(), k + 1, k + 1, L"%d\0", n);
    return string;
}

std::unique_ptr<WCHAR> to_string(double n, int format)
{
    int i = n,  //  int part
        f = 0;  //  frac part
    double d = n - (int)n;

    for (int j = 0; j < format; j++)
        d *= 10;
    f = d;

    if (i == 0)
    {
        std::unique_ptr<WCHAR> string(new WCHAR[format + 3]);
        _snwprintf_s(string.get(), format + 3, format + 3, L"0.%d\0", f);
        return string;
    }
    else
    {
        int t = i,
            k = 0;
        while (t > 0)
        {
            ++k;
            t /= 10;
        }
        std::unique_ptr<WCHAR> string(new WCHAR[k + format + 2]);
        _snwprintf_s(string.get(), k + format + 2, k + format + 2, L"%d.%d\0", i, f);
        return string;
    }
}

double randf()
{
    return (double)rand() / RAND_MAX;
}

double randf(double to)
{
    if (to < 0)
        to *= -1;
    if (to > 1)
        to = to - (int)to;
    return (rand() * to) / RAND_MAX;
}

double randf(double from, double to)
{
    if (from < 0)
        from *= -1;
    if (from > 1)
        from = from - (int)from;
    if (to < 0)
        to *= -1;
    if (to > 1)
        to = to - (int)to;
    if (from > to)
        return from;
    return (rand() * (to - from)) / RAND_MAX + from;
}