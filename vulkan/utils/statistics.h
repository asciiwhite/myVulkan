#pragma once

#include <chrono>
#include <array>

using HistogramData = std::array<float, 10>;

class Statistics
{
public:
    Statistics();

    float getTime() const;
    float getDeltaTime() const;
    float getAverageDeltaTime() const;
    float getAverageFPS() const;

    void update();

    HistogramData const & getDeltaTimeHistogram() const;
    HistogramData const & getFPSHistogram() const;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
    std::chrono::duration<float> m_deltaTime;

    float m_floatTime = 0.f;
    float m_floatDeltaTime = 0.f;
    float m_averageDeltaTime = 0.f;
    float m_averageFPS = 0.f;
    float m_currentSecondFPS = 0.f;
    uint32_t m_frameId = 0;

    HistogramData m_deltaTimeHistogram;
    HistogramData m_FPSHistogram;
};
