#include "deviceref.h"

DeviceRef::DeviceRef(Device& device)
    : m_device(device)
{}

const Device& DeviceRef::device() const
{
    return m_device;
}
