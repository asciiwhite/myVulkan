#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

class Device;

struct Shader
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const { return shaderStageCreateInfo; }

    operator bool() const
    {
        return !shaderModules.empty();
    }

private:
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

    friend class ShaderManager;
};

class ShaderManager
{
public:
    struct ModuleDesc
    {
        VkShaderStageFlagBits stage;
        std::string filename;
    };

    static Shader Acquire(Device& device, const std::vector<ModuleDesc>& modules);
    static void Release(Device& device, Shader& shader);

private:
    static Shader CreateFromFiles(Device& device, const std::vector<ModuleDesc>& modules);
    static VkShaderModule CreateShaderModule(Device& device, const std::string& filename);

    struct ShaderData
    {
        uint64_t refCount;
        Shader shader;
    };

    using ShaderMap = std::unordered_map<std::string, ShaderData>;
    static ShaderMap m_loadedShaders;
};