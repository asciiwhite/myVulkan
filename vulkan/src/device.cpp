#include "device.h"
#include "vulkanhelper.h"
#include "buffer.h"
#include "debug.h"
#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "barrier.h"
#include "commandbuffer.h"

#include <array>
#include <cstring>

bool Device::init(VkInstance instance, VkSurfaceKHR surface, bool enableValidationLayers)
{
    uint32_t numDevices = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &numDevices, nullptr));
    if (numDevices == 0)
    {
        std::cout << "Error occurred during physical devices enumeration!" << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices(numDevices);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &numDevices, &physicalDevices[0]));

    for (uint32_t i = 0; i < numDevices; ++i)
    {
        if (checkPhysicalDeviceProperties(physicalDevices[i], surface))
        {
            m_physicalDevice = physicalDevices[i];
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        std::cout << "Could not select physical device based on the chosen properties!" << std::endl;
        return false;
    }

    auto queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,     // VkStructureType              sType
        nullptr,                                        // const void                  *pNext
        0,                                              // VkDeviceQueueCreateFlags     flags
        m_graphicsQueueFamilyIndex,                     // uint32_t                     queueFamilyIndex
        1,                                              // uint32_t                     queueCount
        &queuePriority                                  // const float                 *pQueuePriorities
    };

    std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures requiredFeatures = {};
    requiredFeatures.robustBufferAccess = enableValidationLayers;

    VkDeviceCreateInfo deviceCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // VkStructureType                    sType
        nullptr,                                        // const void                        *pNext
        0,                                              // VkDeviceCreateFlags                flags
        1,                                              // uint32_t                           queueCreateInfoCount
        &queueCreateInfo,                               // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
        0,                                              // uint32_t                           enabledLayerCount
        nullptr,                                        // const char * const                *ppEnabledLayerNames
        static_cast<uint32_t>(extensions.size()),       // uint32_t                           enabledExtensionCount
        &extensions[0],                                 // const char * const                *ppEnabledExtensionNames
        &requiredFeatures                                // const VkPhysicalDeviceFeatures    *pEnabledFeatures
    };

    if (enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(debug::validationLayerNames.size());
        deviceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames.data();
    }

    VK_CHECK_RESULT(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));

    vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);
    vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_computeQueueFamilyIndex, 0, &m_computeQueue);

    createCommandPools();

    return true;
}

bool Device::checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceProperties(physicalDevice, &m_deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &m_deviceFeatures);

    const uint32_t major_version = VK_VERSION_MAJOR(m_deviceProperties.apiVersion);

    if ((major_version < 1) || (m_deviceProperties.limits.maxImageDimension2D < 4096))
    {
        std::cout << "Physical device " << physicalDevice << " doesn't support required parameters!" << std::endl;
        return false;
    }

    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
    if (queueCount == 0)
    {
        std::cout << "Physical device " << physicalDevice << " doesn't have any queue families!" << std::endl;
        return false;
    }

    std::vector<VkQueueFamilyProperties>  queueProps(queueCount);
    std::vector<VkBool32>                 supportsPresent(queueCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, &queueProps[0]);

    m_graphicsQueueFamilyIndex = UINT32_MAX;
    m_presentQueueFamilyIndex = UINT32_MAX;
    m_computeQueueFamilyIndex = UINT32_MAX;

    // Some devices have dedicated compute queues, so we first try to find a queue that supports compute and not graphics
    bool computeQueueFound = false;
    for (uint32_t i = 0; i < queueCount; i++)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
        {
            m_computeQueueFamilyIndex = i;
            computeQueueFound = true;
            break;
        }
    }

    for (uint32_t i = 0; i < queueCount; ++i)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);

        if ((queueProps[i].queueCount > 0) &&
            (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            // Select first queue that supports graphics
            if (m_graphicsQueueFamilyIndex == UINT32_MAX)
            {
                m_graphicsQueueFamilyIndex = i;
            }
        }

        if ((queueProps[i].queueCount > 0) &&
            (queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            // Select first queue that supports compute
            if (m_computeQueueFamilyIndex == UINT32_MAX)
            {
                m_computeQueueFamilyIndex = i;
            }
        }

        // If there is queue that supports both graphics and present - prefer it
        if (supportsPresent[i])
        {
            m_graphicsQueueFamilyIndex = i;
            m_presentQueueFamilyIndex = i;
            m_computeQueueFamilyIndex = i;
            return true;
        }
    }

    // We don't have queue that supports both graphics and present so we have to use separate queues
    for (uint32_t i = 0; i < queueCount; ++i)
    {
        if (supportsPresent[i])
        {
            m_presentQueueFamilyIndex = i;
            break;
        }
    }

    // If this device doesn't support queues with graphics and present capabilities don't use it
    if ((m_graphicsQueueFamilyIndex == UINT32_MAX) ||
        (m_presentQueueFamilyIndex  == UINT32_MAX) ||
        (m_computeQueueFamilyIndex  == UINT32_MAX))
    {
        std::cout << "Could not find queue family with required properties on physical device " << physicalDevice << "!" << std::endl;
        return false;
    }

    return true;
}

void Device::createCommandPools()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VK_CHECK_RESULT(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool));

    if (m_graphicsQueueFamilyIndex != m_computeQueueFamilyIndex)
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = m_computeQueueFamilyIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VK_CHECK_RESULT(vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_computeCommandPool));
    }
    else
    {
        m_computeCommandPool = m_graphicsCommandPool;
    }
}

void Device::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
    CommandBuffer commandBuffer(*this, getGraphicsCommandPool());
    commandBuffer.begin();
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, size);
    commandBuffer.end();
    commandBuffer.submitBlocking<SubmissionQueue::Graphics>();
}

bool isDepthAttachment(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkRenderPass Device::createRenderPass(const std::vector<RenderPassAttachmentDescription>& attachmentDescriptions) const
{
    assert(!attachmentDescriptions.empty());

    std::vector<VkAttachmentDescription> attachments;
    attachments.resize(attachmentDescriptions.size());

    std::vector<VkAttachmentReference> attachmentRefs;
    attachmentRefs.resize(attachmentDescriptions.size()); // depth attachment is last one

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;

    int attachmentCount = 0;
    for (const auto& attachmentData : attachmentDescriptions)
    {
        VkAttachmentDescription& attachment = attachments[attachmentCount];
        attachment.format = attachmentData.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = attachmentData.loadOp;
        attachment.storeOp = attachmentData.storeOp;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = attachmentData.initialLayout;
        attachment.finalLayout = attachmentData.finalLayout;

        if (isDepthAttachment(attachmentData.format))
        {
            assert(subpass.pDepthStencilAttachment == nullptr);
            VkAttachmentReference& attachmentRef = attachmentRefs.back();
            attachmentRef.attachment = attachmentCount;
            attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.pDepthStencilAttachment = &attachmentRef;
        }
        else
        {
            VkAttachmentReference& attachmentRef = attachmentRefs[subpass.colorAttachmentCount];
            attachmentRef.attachment = attachmentCount;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpass.colorAttachmentCount++;
        }

        attachmentCount++;
    }

    if (subpass.colorAttachmentCount)
        subpass.pColorAttachments = attachmentRefs.data();

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

 /*
    TODO: dependencies from swap image write to swap image read
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
*/
    // dependencies from color attachment write to color attachment read
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkRenderPass renderPass;
    VK_CHECK_RESULT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass));
    return renderPass;
}

VkFramebuffer Device::createFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent) const
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = &attachments.front();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VkFramebuffer frameBuffer;
    VK_CHECK_RESULT(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &frameBuffer));

    return frameBuffer;
}

VkPipelineLayout Device::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<VkPushConstantRange>& pushConstants) const
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = !layouts.empty() ? layouts.data() : nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    pipelineLayoutInfo.pPushConstantRanges = !pushConstants.empty() ? pushConstants.data() : nullptr;

    VkPipelineLayout pipelineLayout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    return pipelineLayout;
}

VkPipeline Device::createPipeline(VkRenderPass renderPass, VkPipelineLayout layout, const GraphicsPipelineSettings& settings,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
    const std::vector<VkVertexInputBindingDescription>& bindingDesc) const
{
    assert(!shaderStages.empty());

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDesc.empty() ? nullptr : bindingDesc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.empty() ? nullptr : attributeDesc.data();

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = &shaderStages[0];
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &settings.inputAssembly;
    pipelineInfo.pViewportState = &settings.viewportState;
    pipelineInfo.pRasterizationState = &settings.rasterizer;
    pipelineInfo.pMultisampleState = &settings.multisampling;
    pipelineInfo.pDepthStencilState = &settings.depthStencil;
    pipelineInfo.pColorBlendState = &settings.colorBlending;
    pipelineInfo.pDynamicState = &settings.dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    return pipeline;
}

VkDescriptorSetLayout Device::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const
{
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &layout));

    return layout;
}

VkDescriptorPool Device::createDescriptorPool(uint32_t count, const std::vector<VkDescriptorPoolSize>& sizes, bool allowIndividualFreeing) const
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = allowIndividualFreeing ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    poolInfo.pPoolSizes = sizes.data();
    poolInfo.maxSets = count;

    VkDescriptorPool descriptorPool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPool));

    return descriptorPool;
}

VkSampler Device::createSampler(bool clampToEdge) const
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = clampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = clampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = clampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    VK_CHECK_RESULT(vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler));
    return sampler;
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    return findMemoryType(m_physicalDevice, typeFilter, properties);
}

uint32_t Device::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (auto i = 0u; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    assert(!"Could not find requested memory type");
    return ~0u;
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

void Device::copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D resolution) const
{
    CommandBuffer commandBuffer(*this, getGraphicsCommandPool());
    commandBuffer.begin();
    commandBuffer.copyBufferToImage(buffer, image, resolution);
    commandBuffer.end();
    commandBuffer.submitBlocking<SubmissionQueue::Graphics>();
}

void Device::destroy()
{
    if (m_computeCommandPool != m_graphicsCommandPool)
    {
        vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
        m_computeCommandPool = VK_NULL_HANDLE;
    }

    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    m_graphicsCommandPool = VK_NULL_HANDLE;

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
}
