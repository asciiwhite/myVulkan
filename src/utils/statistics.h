#include <chrono>

class Statistics
{
public:
    Statistics();

    void startFrame();
    bool endFrame();
    void reset();

    uint64_t getMin() const;
    uint64_t getMax() const;
    uint64_t getAvg() const;

private:
    using TimeUnit = std::chrono::microseconds;
    using Clock = std::chrono::high_resolution_clock;

    Clock::time_point m_frameStartTime;
    TimeUnit m_minFrameTime;
    TimeUnit m_maxFrameTime;
    TimeUnit m_avgFrameTime;
    TimeUnit m_totalTime;
    uint32_t m_frameCount;
};
