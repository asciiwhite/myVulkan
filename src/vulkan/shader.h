#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class Shader
{
public:
    ~Shader();

    std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const { return m_shaderStages; }

    static std::shared_ptr<Shader> getShader(VkDevice device, const std::string& vertexFilename, const std::string& fragmentFilename);
    static void release(std::shared_ptr<Shader>& shader);
private:
    bool createFromFiles(VkDevice device, const std::string& vertexFilename, const std::string& fragmentFilename);

    static VkShaderModule CreateShaderModule(VkDevice device, const std::string& filename);

    VkDevice m_device = VK_NULL_HANDLE;
    std::vector<VkShaderModule> m_shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

    using ShaderMap = std::unordered_map<std::string, std::shared_ptr<Shader>>;
    static ShaderMap m_loadedShaders;
};