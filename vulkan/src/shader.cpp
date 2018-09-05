#include "shader.h"
#include "vulkanhelper.h"
#include "device.h"

#include <fstream>
#include <algorithm>

ShaderManager::ShaderMap ShaderManager::m_loadedShaders;

Shader ShaderManager::Acquire(Device& device, const std::vector<ModuleDesc>& modules)
{
    std::string combindedShaderFilenames;

    for (const auto& desc : modules)
        combindedShaderFilenames += desc.filename;

    if (m_loadedShaders.count(combindedShaderFilenames) == 0)
    {
        Shader newShader = CreateFromFiles(device, modules);
        if (newShader)
        {
            m_loadedShaders[combindedShaderFilenames] = { 1, newShader }; // init refcount
        }
        return newShader;
    }

    auto& entry = m_loadedShaders.at(combindedShaderFilenames);
    entry.refCount++;
    return entry.shader;
}

void ShaderManager::Release(Device& device, Shader& shader)
{
    auto iter = std::find_if(m_loadedShaders.begin(), m_loadedShaders.end(), [=](const ShaderMap::value_type& shaderPair) { return shaderPair.second.shader == shader; });
    assert(iter != m_loadedShaders.end());

    auto& entry = iter->second;
    if (--entry.refCount == 0)
    {
        for (auto shaderModule : entry.shader.shaderModules)
        {
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
        m_loadedShaders.erase(iter);
    }
}

Shader ShaderManager::CreateFromFiles(Device& device, const std::vector<ModuleDesc>& modules)
{
    Shader shader;

    for (const auto& moduleDesc : modules)
    {
        auto shaderModule = CreateShaderModule(device, moduleDesc.filename);
        if (shaderModule == VK_NULL_HANDLE)
            return shader;

        shader.shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = moduleDesc.stage;
        info.module = shaderModule;
        info.pName = "main";

        shader.shaderStageCreateInfo.push_back(info);
    }

    return shader;
}

VkShaderModule ShaderManager::CreateShaderModule(Device& device, const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return VK_NULL_HANDLE;
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}