#pragma once

class Device;

class DeviceRef
{
public:
    const Device& device() const;

    template<typename T>
    void destroy(T t) const;

protected:
    DeviceRef() = delete;
    DeviceRef(Device& device);

private:
    const Device& m_device;
};
