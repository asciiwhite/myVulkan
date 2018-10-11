#pragma once

class Device;

class DeviceRef
{
public:
    const Device& device() const;

    template<typename T>
    void destroy(T t) const;

    void swap(DeviceRef& device);

protected:
    DeviceRef();
    DeviceRef(const Device& device);

private:
    const Device* m_device;
};
