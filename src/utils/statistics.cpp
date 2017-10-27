#include "statistics.h"

#include <algorithm>

Statistics::Statistics()
{
    reset();
}

void Statistics::reset()
{
    m_minFrameTime = TimeUnit::max();
    m_maxFrameTime = TimeUnit::zero();
    m_avgFrameTime = TimeUnit::zero();
    m_totalTime = TimeUnit::zero();
    m_frameCount = 0;
}

void Statistics::startFrame()
{
    m_frameStartTime = Clock::now();
}

bool Statistics::endFrame()
{
    const auto totalFrameTime = Clock::now() - m_frameStartTime;
    const auto totalFrameTimeInUnits = std::chrono::duration_cast<TimeUnit>(totalFrameTime);

    m_frameCount++;
    m_minFrameTime = std::min(m_minFrameTime, totalFrameTimeInUnits);
    m_maxFrameTime = std::max(m_maxFrameTime, totalFrameTimeInUnits);
    m_totalTime += totalFrameTimeInUnits;
    

    if (m_totalTime >= std::chrono::seconds(1)) 
    {
        m_avgFrameTime = m_totalTime / m_frameCount;
        return true;
    }

    return false;
}

uint64_t Statistics::getMin() const
{
    return m_minFrameTime.count();
}

uint64_t Statistics::getMax() const
{
    return m_maxFrameTime.count();
}

uint64_t Statistics::getAvg() const
{
    return m_avgFrameTime.count();
}
