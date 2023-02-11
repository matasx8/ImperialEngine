#include <iostream>
#include "backend/graphics/VkDebug.h"

imp::ValidationLayers::ValidationLayers()
    : m_DebugMessenger()
{
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "[VULKAN]: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void imp::ValidationLayers::EnableCallback(VkInstance instance, const EngineGraphicsSettings& settings)
{
    // TODO: The application must ensure that vkDestroyDebugUtilsMessengerEXT is not executed in parallel with any Vulkan command that is also called with instance or child of instance as the dispatchable argument.
    // TODO: Configure message severity from settings
    const auto createInfo = CreateDebugMessengerCreateInfo(settings);
    CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_DebugMessenger);
}

VkDebugUtilsMessengerCreateInfoEXT imp::ValidationLayers::CreateDebugMessengerCreateInfo(const EngineGraphicsSettings& settings)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT /* | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT */| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    return createInfo;
}

void imp::ValidationLayers::Destroy(VkInstance instance)
{
    DestroyDebugUtilsMessengerEXT(instance, m_DebugMessenger, nullptr);
}

