#pragma once

#include <chrono>

class Timer
{
public:
    using TimeUnit = std::chrono::microseconds;
    using Clock = std::chrono::high_resolution_clock;

    static uint64_t getMicroseconds()
    {
        const auto now = Clock::now();
        return std::chrono::time_point_cast<TimeUnit>(now).time_since_epoch().count();
    }
};

