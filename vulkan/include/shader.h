#pragma once

#include "handles.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

class Shader
{
public:
    ~Shader();

    std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const { return m_shaderStageCreateInfo; }

    struct ModuleDesc
    {
        VkShaderStageFlagBits stage;
        std::string filename;
    };

    static ShaderHandle getShader(VkDevice device, const std::vector<ModuleDesc>& modules);
    static void release(ShaderHandle& shader);
private:
    bool createFromFiles(VkDevice device, const std::vector<ModuleDesc>& modules);

    static VkShaderModule CreateShaderModule(VkDevice device, const std::string& filename);

    VkDevice m_device = VK_NULL_HANDLE;
    std::vector<VkShaderModule> m_shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfo;

    using ShaderMap = std::unordered_map<std::string, ShaderHandle>;
    static ShaderMap m_loadedShaders;
};