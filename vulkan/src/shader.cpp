#include "shader.h"
#include "vulkanhelper.h"

#include <fstream>
#include <algorithm>

Shader::ShaderMap Shader::m_loadedShaders;

ShaderHandle Shader::getShader(VkDevice device, const std::string& vertexFilename, const std::string& fragmentFilename)
{
    const auto combindedShaderFilenames = vertexFilename + fragmentFilename;
    if (m_loadedShaders.count(combindedShaderFilenames) == 0)
    {
        auto newShader = std::make_shared<Shader>();
        if (newShader->createFromFiles(device, vertexFilename, fragmentFilename))
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

bool Shader::createFromFiles(VkDevice device, const std::string& vertexFilename, const std::string& fragmentFilename)
{
    m_device = device;

    m_shaderModules.resize(2);
    m_shaderModules[0] = CreateShaderModule(device, vertexFilename);
    if (m_shaderModules[0] == VK_NULL_HANDLE)
        return false;

    m_shaderModules[1] = CreateShaderModule(device, fragmentFilename);
    if (m_shaderModules[1] == VK_NULL_HANDLE)
        return false;

    m_shaderStages.resize(2);
    m_shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    m_shaderStages[0].module = m_shaderModules[0];
    m_shaderStages[0].pName = "main";

    m_shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_shaderStages[1].module = m_shaderModules[1];
    m_shaderStages[1].pName = "main";

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