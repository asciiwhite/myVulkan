#include "scopedtimelog.h"

#include <iostream>

ScopedTimeLog::ScopedTimeLog(const std::string name)
    : m_startTime(Timer::getMicroseconds())
    , m_name(name)
{
}

ScopedTimeLog::~ScopedTimeLog()
{
    const auto totalTime = Timer::getMicroseconds() - m_startTime;
    std::cout << m_name << ":\t " << totalTime / 1000 << " ms" << std::endl;
}
