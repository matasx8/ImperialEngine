#include "backend/graphics/GraphicsCaps.h"
#include "backend/EnumTranslator.h"
#include <vector>

imp::GraphicsCaps::GraphicsCaps()
{
}

bool imp::GraphicsCaps::ValidationLayersSupported()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) 
        if (strcmp(validationLayerName, layerProperties.layerName) == 0)
            return true;

    printf("Warning: No available and suitable validaiton layers found!\n");
    return false;
}

imp::QueueFamilyIndices imp::GraphicsCaps::GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    int i = 0;
    // go through each queue family and check if it has at least one of the required types of queue
    for (const auto& queueFamily : queueFamilyList)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        // check if queue family supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
        if (queueFamily.queueCount > 0 && presentationSupport)
            indices.presentationFamily = i;

        if (indices.IsValid())
            break;
        i++;
    }

    return indices;
}

const imp::PhysicalDeviceSurfaceCaps& imp::GraphicsCaps::GetPhysicalDeviceSurfaceCaps() const
{
    return m_DeviceSurfaceCaps;
}

imp::QueueFamilyIndices imp::GraphicsCaps::GetQueueFamilies() const
{
    return m_QueueFamilyIndices;
}

void imp::GraphicsCaps::SetQueueFamilies(QueueFamilyIndices& fams)
{
    m_QueueFamilyIndices = fams;
}

VkSurfaceFormatKHR imp::GraphicsCaps::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM,  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR imp::GraphicsCaps::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes, const EngineGraphicsSettings& settings)
{
    for (const auto& preferred : settings.preferredPresentModes)
        for (const auto& presentationMode : presentationModes)
            if (presentationMode == TranslatePresentMode(preferred))
                return presentationMode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D imp::GraphicsCaps::ChooseBestExtent(VkSurfaceCapabilitiesKHR surfaceCaps, VkExtent2D preferredExtent)
{
    // if current extent is at numeric limits, then extent can vary
    if (surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surfaceCaps.currentExtent;
    else
    {
        preferredExtent.width = std::max(surfaceCaps.minImageExtent.width, std::min(surfaceCaps.maxImageExtent.width, preferredExtent.width));
        preferredExtent.height = std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, preferredExtent.height));

        return preferredExtent;
    }
}

uint32_t imp::GraphicsCaps::ChooseSwapchainImageCount(VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t preferred)
{
    if (surfaceCaps.maxImageCount == 0)
        return std::max(surfaceCaps.minImageCount, preferred);
    return std::max(surfaceCaps.minImageCount, std::min(surfaceCaps.maxImageCount, preferred));
}

bool imp::GraphicsCaps::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = GetQueueFamilies(device, surface);
    return indices.IsValid() && deviceFeatures.samplerAnisotropy && CheckDeviceHasAnySwapchainSupport(device, surface);
}

bool imp::GraphicsCaps::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtens)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0)
        return false;

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    for (const auto& deviceExtension : requiredExtens)
    {
        bool hasExtension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
            return false;
    }

    return true;
}

bool imp::GraphicsCaps::CheckDeviceHasAnySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &m_DeviceSurfaceCaps.surfaceCapabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        m_DeviceSurfaceCaps.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, m_DeviceSurfaceCaps.formats.data());
    }

    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);
    if (presentationCount != 0)
    {
        m_DeviceSurfaceCaps.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, m_DeviceSurfaceCaps.presentationModes.data());
    }

    return m_DeviceSurfaceCaps.presentationModes.size() && m_DeviceSurfaceCaps.formats.size();
}

uint32_t imp::MemoryProps::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags flags)
{
    // TODO: I'm not sure if this function makes much sense
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return ~0u;
}
