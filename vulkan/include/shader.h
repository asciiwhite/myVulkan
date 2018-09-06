#pragma once

#include "resourcemanager.h"

#include <vulkan/vulkan.h>
#include <string>

class Device;

struct Shader
{
public:
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

    operator bool() const
    {
        return !shaderModules.empty();
    }
};

class ShaderResourceHandler
{
public:
    using ResourceKey = std::string;
    using ResourceType = Shader;

    struct ModuleDesc
    {
        VkShaderStageFlagBits stage;
        std::string filename;
    };
    using ShaderModulesDescription = std::vector<ModuleDesc>;

    static ResourceKey CreateResourceKey(const ShaderModulesDescription& modules);
    static ResourceType CreateResource(Device& device, const ShaderModulesDescription& modules);
    static void DestroyResource(Device& device, ResourceType& resource);

private:
    static Shader CreateFromFiles(Device& device, const ShaderModulesDescription& modules);
    static VkShaderModule CreateShaderModule(Device& device, const std::string& filename);
};

using ShaderManager = ResourceManager<ShaderResourceHandler>;
