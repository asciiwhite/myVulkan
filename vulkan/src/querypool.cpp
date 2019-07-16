#include "querypool.h"
#include "device.h"
#include "vulkanhelper.h"

QueryPool::QueryPool(const Device& device)
    : DeviceRef(device)
{}

void QueryPool::init()
{
    VkQueryPoolCreateInfo queryPoolInfo = {};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = 2;
    VK_CHECK_RESULT(vkCreateQueryPool(device(), &queryPoolInfo, nullptr, &m_queryPool));
}

void QueryPool::destroy()
{
    vkDestroyQueryPool(device(), m_queryPool, nullptr);
}

void QueryPool::begin(VkCommandBuffer commandBuffer) const
{
    assert(m_queryPool);

    vkCmdResetQueryPool(commandBuffer, m_queryPool, 0, 2);
    // TODO: maybe VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT?
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, 0);
}

void QueryPool::end(VkCommandBuffer commandBuffer) const
{
    assert(m_queryPool);

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, 1);
}

std::chrono::microseconds QueryPool::duration() const
{
    assert(m_queryPool);

    uint64_t timeStamps[2];
    if (vkGetQueryPoolResults(device(), m_queryPool, 0, 2, sizeof(uint64_t) * 2, timeStamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) != VK_SUCCESS)
        return std::chrono::microseconds(0);

    const auto diff = timeStamps[1] - timeStamps[0];
    const auto nanoseconds = diff * device().properties().limits.timestampPeriod;
    return std::chrono::microseconds(static_cast<uint64_t>(nanoseconds / 1000));
}
