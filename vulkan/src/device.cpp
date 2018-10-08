#include "device.h"
#include "vulkanhelper.h"
#include "buffer.h"
#include "debug.h"
#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "texture.h"

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
        deviceCreateInfo.enabledLayerCount = debug::validationLayerCount;
        deviceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames;
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
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures   device_features;

    vkGetPhysicalDeviceProperties(physicalDevice, &device_properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &device_features);

    const uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);

    if ((major_version < 1) || (device_properties.limits.maxImageDimension2D < 4096))
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
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

Buffer Device::createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const
{
    Buffer resultBuffer;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(m_device, &bufferInfo, nullptr, &resultBuffer.buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, resultBuffer.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, properties);

    vkAllocateMemory(m_device, &allocInfo, nullptr, &resultBuffer.memory);
    vkBindBufferMemory(m_device, resultBuffer.buffer, resultBuffer.memory, 0);

    return resultBuffer;
}

void Device::destroyBuffer(Buffer& buffer) const
{
    vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    vkFreeMemory(m_device, buffer.memory, nullptr);

    buffer.buffer = VK_NULL_HANDLE;
    buffer.memory = VK_NULL_HANDLE;
}

void* Device::mapBuffer(const Buffer& buffer, uint64_t size, uint64_t offset) const
{
    void* data;
    VK_CHECK_RESULT(vkMapMemory(m_device, buffer.memory, offset, size, 0, &data));
    return data;
}

void Device::unmapBuffer(const Buffer& buffer) const
{
    vkUnmapMemory(m_device, buffer.memory);
}

VkRenderPass Device::createRenderPass(const std::array<RenderPassAttachmentData, 2>& attachmentData) const
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = attachmentData[0].format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = attachmentData[0].loadOp;
    colorAttachment.storeOp = attachmentData[0].storeOp;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = attachmentData[0].initialLayout;
    colorAttachment.finalLayout = attachmentData[0].finalLayout;
 
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = attachmentData[1].format;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = attachmentData[1].loadOp;
    depthAttachment.storeOp = attachmentData[1].storeOp;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = attachmentData[1].initialLayout;
    depthAttachment.finalLayout = attachmentData[1].finalLayout;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.preserveAttachmentCount = 0;
    subpass.pResolveAttachments = nullptr;
    subpass.pPreserveAttachments = nullptr;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    VkRenderPass renderPass;
    VK_CHECK_RESULT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass));
    return renderPass;
}

void Device::destroyRenderPass(VkRenderPass& renderPass) const
{
    vkDestroyRenderPass(m_device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
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

void Device::destroyFramebuffer(VkFramebuffer& framebuffer) const
{
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
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

void Device::destroyPipelineLayout(VkPipelineLayout& pipelineLayout) const
{
    vkDestroyPipelineLayout(m_device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
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

void Device::destroyPipeline(VkPipeline& pipeline) const
{
    vkDestroyPipeline(m_device, pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;
}


Texture Device::createDepthBuffer(const VkExtent2D& extend, VkFormat format) const
{
    Texture texture;

    createImage(extend.width, extend.height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture.image, texture.imageMemory);

    createImageView(texture.image, format, texture.imageView, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(texture.image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return texture;
}

Texture Device::createImageFromData(uint32_t width, uint32_t height, unsigned char* pixelData, VkFormat format) const
{
    assert(format == VK_FORMAT_R8G8B8A8_UNORM);

    const uint32_t imageSize = width * height * 4;

    Texture texture;
    Buffer stagingBuffer = createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data = mapBuffer(stagingBuffer, imageSize, 0);
    std::memcpy(data, pixelData, static_cast<size_t>(imageSize));
    unmapBuffer(stagingBuffer);

    createImage(width, height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture.image, texture.imageMemory);

    transitionImageLayout(texture.image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuffer, texture.image, width, height);

    transitionImageLayout(texture.image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    destroyBuffer(stagingBuffer);

    createImageView(texture.image, format, texture.imageView, VK_IMAGE_ASPECT_COLOR_BIT);

    return texture;
}

void Device::destroyTexture(Texture& texture) const
{
    vkDestroyImageView(m_device, texture.imageView, nullptr);
    texture.imageView = VK_NULL_HANDLE;

    vkDestroyImage(m_device, texture.image, nullptr);
    texture.image = VK_NULL_HANDLE;

    vkFreeMemory(m_device, texture.imageMemory, nullptr);
    texture.imageMemory = VK_NULL_HANDLE;
}

void Device::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateImage(m_device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(m_device, image, imageMemory, 0));
}

VkSampler Device::createSampler() const
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

void Device::destroySampler(VkSampler& sampler) const
{
    vkDestroySampler(m_device, sampler, nullptr);
    sampler = VK_NULL_HANDLE;
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

bool Device::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static VkPipelineStageFlags getPipelineStageFlags(VkAccessFlags access)
{
    if (access & (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT))
    {
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT/*| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT*/;
    }
    if (access & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT))
    {
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (access & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
    {
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    if ((access & ~(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT)) == 0)
    {
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    assert(!"Unknown access flags.");
    return 0;
}

static VkAccessFlags getAccessFlags(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;

    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_ACCESS_MEMORY_READ_BIT;

    case VK_IMAGE_LAYOUT_GENERAL: // assume storage image
        return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    default:
        break;
    }

    assert(!"Unsupported layout transition.");
    return 0;
}

void Device::transitionImageLayout(VkImage image, VkFormat imageFormat, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = getAccessFlags(oldLayout);
    barrier.dstAccessMask = getAccessFlags(newLayout);

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(imageFormat))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    const auto sourceStage = getPipelineStageFlags(barrier.srcAccessMask);
    const auto destinationStage = getPipelineStageFlags(barrier.dstAccessMask);

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void Device::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Device::beginSingleTimeCommands() const
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK_RESULT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK_RESULT(vkQueueWaitIdle(m_graphicsQueue));

    vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);
}

void Device::createImageView(VkImage image, VkFormat format, VkImageView& imageView, VkImageAspectFlags aspectFlags) const
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.format = format;
    viewInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.flags = 0;
    viewInfo.image = image;

    VK_CHECK_RESULT(vkCreateImageView(m_device, &viewInfo, nullptr, &imageView));
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
