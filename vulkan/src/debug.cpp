#include "debug.h"
#include "vulkanhelper.h"

namespace debug
{
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = VK_NULL_HANDLE;
    PFN_vkDebugReportMessageEXT DebugReportCallback = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT dbgCallback = VK_NULL_HANDLE;

    VkBool32 debugCallback(
        VkDebugReportFlagsEXT /*flags*/,
        VkDebugReportObjectTypeEXT /*objType*/,
        uint64_t /*obj*/,
        size_t /*location*/,
        int32_t /*code*/,
        const char* /*layerPrefix*/,
        const char* msg,
        void* /*userData*/)
    {
        std::cerr << "validation layer: " << msg << std::endl;
        return VK_FALSE;
    }

    void setupDebugCallback(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportCallbackEXT callBack)
    {
        CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
        DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
        DebugReportCallback = reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));

        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        dbgCreateInfo.flags = flags;
        dbgCreateInfo.pfnCallback = debugCallback;

        VK_CHECK_RESULT(CreateDebugReportCallback(instance, &dbgCreateInfo, nullptr, (callBack != VK_NULL_HANDLE) ? &callBack : &dbgCallback));
    }

    void destroyDebugCallback(VkInstance instance)
    {
        if (dbgCallback != VK_NULL_HANDLE)
        {
            DestroyDebugReportCallback(instance, dbgCallback, nullptr);
        }
    }
}
