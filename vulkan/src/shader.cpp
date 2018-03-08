#include "shader.h"
#include "vulkanhelper.h"

#include <fstream>
#include <algorithm>

Shader::ShaderMap Shader::m_loadedShaders;

ShaderHandle Shader::getShader(VkDevice device, const std::vector<ModuleDesc>& modules)
{
    std::string combindedShaderFilenames;

    for (const auto& desc : modules)
        combindedShaderFilenames += desc.filename;

    if (m_loadedShaders.count(combindedShaderFilenames) == 0)
    {
        auto newShader = std::make_shared<Shader>();
        if (newShader->createFromFiles(device, modules))
        {
            m_loadedShaders[combindedShaderFilenames] = newShader;
        }
        else
        {
            newShader.reset();
        }
        return newShader;
    }

    return m_loadedShaders[combindedShaderFilenames];
}

void Shader::release(ShaderHandle& shader)
{
    auto iter = std::find_if(m_loadedShaders.begin(), m_loadedShaders.end(), [=](const ShaderMap::value_type& shaderPair) { return shaderPair.second == shader; });
    assert(iter != m_loadedShaders.end());

    shader.reset();

    if (iter->second.unique())
    {
        m_loadedShaders.erase(iter);
    }
}

Shader::~Shader()
{
    for (auto shaderModule : m_shaderModules)
    {
        vkDestroyShaderModule(m_device, shaderModule, nullptr);
    }
}

bool Shader::createFromFiles(VkDevice device, const std::vector<ModuleDesc>& modules)
{
    m_device = device;

    for (const auto& moduleDesc : modules)
    {
        auto shaderModule = CreateShaderModule(device, moduleDesc.filename);
        if (shaderModule == VK_NULL_HANDLE)
            return false;

        m_shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = moduleDesc.stage;
        info.module = shaderModule;
        info.pName = "main";

        m_shaderStageCreateInfo.push_back(info);
    }

    return true;
}

VkShaderModule Shader::CreateShaderModule(VkDevice device, const std::string& filename)
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