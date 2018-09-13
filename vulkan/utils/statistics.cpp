#include "statistics.h"

Statistics::Statistics()
    : m_time(std::chrono::high_resolution_clock::now())
    , m_deltaTime(std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now())
    , m_floatDeltaTime(10.0f)
    , m_averageDeltaTime(10.0f)
    , m_averageFPS(10.0f)
    , m_currentSecondFPS(10.0f)
{
    m_FPSHistogram.fill(0.f);
    m_deltaTimeHistogram.fill(0.0f);

    update();
}

float Statistics::getTime() const
{
    return m_floatTime;
}

float Statistics::getDeltaTime() const
{
    return m_floatDeltaTime;
}

float Statistics::getAverageDeltaTime() const
{
    return m_averageDeltaTime;
}

HistogramData const & Statistics::getDeltaTimeHistogram() const
{
    return m_deltaTimeHistogram;
}

float Statistics::getAverageFPS() const
{
    return m_averageFPS;
}

HistogramData const & Statistics::getFPSHistogram() const
{
    return m_FPSHistogram;
}

void Statistics::update()
{
    auto previousTime = m_time;
    m_time = std::chrono::high_resolution_clock::now();
    m_deltaTime = std::chrono::high_resolution_clock::now() - previousTime;

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(m_time.time_since_epoch()).count();
    m_floatTime = static_cast<float>(milliseconds * 0.001f);
    m_floatDeltaTime = m_deltaTime.count();

    static size_t previous_second = 0;
    size_t current_second = static_cast<size_t>(m_floatTime) % (2);

    if (current_second != previous_second)
    {
        m_averageFPS = m_currentSecondFPS;
        for (size_t i = 1; i < m_FPSHistogram.size(); ++i)
        {
            m_averageFPS += m_FPSHistogram[i];
            m_FPSHistogram[i - 1] = m_FPSHistogram[i];
            m_deltaTimeHistogram[i - 1] = m_deltaTimeHistogram[i];
        }
        m_FPSHistogram[9] = m_currentSecondFPS;
        m_deltaTimeHistogram[9] = 1000.0f / m_currentSecondFPS;
        m_averageFPS *= 0.1f;
        m_averageDeltaTime = 1000.0f / m_averageFPS;
        m_currentSecondFPS = 0.0f;
    }
    m_currentSecondFPS += 1.0f;
    previous_second = current_second;
}
