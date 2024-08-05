#pragma once

#include <Windows.h>
#include <stdio.h>
#include <memory>

std::unique_ptr<WCHAR> to_string(int n);
std::unique_ptr<WCHAR> to_string(double n, int format);
double randf();
double randf(double to);
double randf(double from, double to);

class MutexLocker
{
    HANDLE hMutex;
public:
    MutexLocker(HANDLE mutex)
        :hMutex(mutex)
    {
        WaitForSingleObject(hMutex, INFINITE);
    }
    ~MutexLocker()
    {
        ReleaseMutex(hMutex);
    }
};