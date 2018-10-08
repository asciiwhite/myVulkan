#pragma once

#include <unordered_map>
#include <algorithm>
#include <assert.h>

class Device;

template <typename ResourceHandler>
class ResourceManager
{
public:
    template<typename... Args>
    static typename ResourceHandler::ResourceType Acquire(const Device& device, Args... args)
    {
        const auto resourceKey = ResourceHandler::CreateResourceKey(args...);
        if (m_createdResources.count(resourceKey) == 0)
        {
            auto newResource = ResourceHandler::CreateResource(device, args...);
            if (newResource)
            {
                m_createdResources[resourceKey] = { 1, newResource }; // init refcount
            }
            return newResource;
        }

        auto& entry = m_createdResources.at(resourceKey);
        entry.refCount++;
        return entry.resource;
    }

    static void Release(const Device& device, typename ResourceHandler::ResourceType& resource)
    {
        auto iter = std::find_if(m_createdResources.begin(), m_createdResources.end(), [=](const typename ResourceMap::value_type& resourcePair) { return resourcePair.second.resource == resource; });
        assert(iter != m_createdResources.end());

        auto& entry = iter->second;
        if (--entry.refCount == 0)
        {
            ResourceHandler::DestroyResource(device, entry.resource);
            m_createdResources.erase(iter);
        }
    }

private:
    struct RefCountedResource
    {
        size_t refCount;
        typename ResourceHandler::ResourceType resource;
    };

    using ResourceMap = std::unordered_map<typename ResourceHandler::ResourceKey, RefCountedResource>;
    static ResourceMap m_createdResources;
};

template <typename ResourceHandler>
typename ResourceManager<ResourceHandler>::ResourceMap ResourceManager<ResourceHandler>::m_createdResources;