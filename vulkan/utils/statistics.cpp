#include "statistics.h"
#include "timer.h"

#include <algorithm>

Statistics::Statistics()
{
    reset();
}

void Statistics::reset()
{
    m_minFrameTime = std::numeric_limits<uint64_t>::max();
    m_maxFrameTime = 0u;
    m_avgFrameTime = 0u;
    m_totalTime = 0u;
    m_frameCount = 0;
}

void Statistics::startFrame()
{
    m_frameStartTime = Timer::getMicroseconds();
}

bool Statistics::endFrame()
{
    const auto totalFrameTime = Timer::getMicroseconds() - m_frameStartTime;

    m_frameCount++;
    m_minFrameTime = std::min(m_minFrameTime, totalFrameTime);
    m_maxFrameTime = std::max(m_maxFrameTime, totalFrameTime);
    m_totalTime += totalFrameTime;

    if (m_totalTime >= static_cast<uint64_t>(std::chrono::duration_cast<Timer::TimeUnit>(std::chrono::seconds(1)).count()))
    {
        m_avgFrameTime = m_totalTime / m_frameCount;
        return true;
    }

    return false;
}

uint64_t Statistics::getMin() const
{
    return m_minFrameTime;
}

uint64_t Statistics::getMax() const
{
    return m_maxFrameTime;
}

uint64_t Statistics::getAvg() const
{
    return m_avgFrameTime;
}
