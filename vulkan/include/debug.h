#include <vulkan/vulkan.h>
#include <array>

namespace debug
{
    const std::array<const char*, 1> validationLayerNames{ "VK_LAYER_LUNARG_standard_validation" };

    VkBool32 debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* layerPrefix,
        const char* msg,
        void* userData);

    void setupDebugCallback(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportCallbackEXT callBack);
    void destroyDebugCallback(VkInstance instance);
}
