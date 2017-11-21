#pragma once

#include "timer.h"

#include <string>

class ScopedTimeLog
{
public:
    ScopedTimeLog(const std::string& name);
    ~ScopedTimeLog();

private:
    uint64_t m_startTime;
    const std::string m_name;
};
