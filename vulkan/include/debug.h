#include <vulkan/vulkan.h>

namespace debug
{
    extern int32_t validationLayerCount;
    extern const char* validationLayerNames[];

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
