#include "shader.h"
#include "vulkanhelper.h"
#include "device.h"

#include <fstream>
#include <algorithm>

std::string ShaderResourceHandler::CreateResourceKey(const ShaderModulesDescription& modules)
{
    std::string combindedShaderFilenames;

    for (const auto& desc : modules)
        combindedShaderFilenames += desc.filename;

    return combindedShaderFilenames;
}

Shader ShaderResourceHandler::CreateResource(const Device& device, const ShaderModulesDescription& modules)
{
    return CreateFromFiles(device, modules);
}

void ShaderResourceHandler::DestroyResource(const Device& device, Shader& shader)
{
    for (auto& shaderModule : shader.shaderModules)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        shaderModule = VK_NULL_HANDLE;
    }
    shader.shaderModules.clear();
    shader.shaderStageCreateInfos.clear();
}

Shader ShaderResourceHandler::CreateFromFiles(const Device& device, const ShaderModulesDescription& modules)
{
    Shader shader;

    for (const auto& moduleDesc : modules)
    {
        auto shaderModule = CreateShaderModule(device, moduleDesc.filename);
        if (shaderModule == VK_NULL_HANDLE)
        {
            DestroyResource(device, shader);
            return shader;
        }

        shader.shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = moduleDesc.stage;
        info.module = shaderModule;
        info.pName = "main";

        shader.shaderStageCreateInfos.push_back(info);
    }

    return shader;
}

VkShaderModule ShaderResourceHandler::CreateShaderModule(const Device& device, const std::string& filename)
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

    // TODO: shader modules can be destroyed after pipeline was created with according VkPipelineShaderStageCreateInfo
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}
