#include <stdint.h>

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
    uint64_t m_frameStartTime;
    uint64_t m_minFrameTime;
    uint64_t m_maxFrameTime;
    uint64_t m_avgFrameTime;
    uint64_t m_totalTime;
    uint32_t m_frameCount;
};
