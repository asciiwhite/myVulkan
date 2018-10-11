#include "deviceref.h"

#include <assert.h>
#include <utility>

DeviceRef::DeviceRef(const Device& device)
    : m_device(&device)
{}

DeviceRef::DeviceRef()
    : m_device(nullptr)
{}

const Device& DeviceRef::device() const
{
    assert(m_device != nullptr);
    return *m_device;
}

void DeviceRef::swap(DeviceRef& other)
{
    std::swap(m_device, other.m_device);
}