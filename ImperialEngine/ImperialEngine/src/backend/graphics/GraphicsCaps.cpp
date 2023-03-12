#include "backend/graphics/GraphicsCaps.h"
#include "backend/EnumTranslator.h"
#include <vector>

imp::GraphicsCaps::GraphicsCaps() 
    : m_DeviceSurfaceCaps(),
    m_QueueFamilyIndices(),
    m_MeshShadingSupported(true)
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

inline int GetDesiredQueue(std::vector<VkQueueFamilyProperties>& fams, VkQueueFlags desiredFlags, VkQueueFlags undesiredFlags)
{
    for (uint32_t i = 0; const auto& fam : fams)
    {
        if (fam.queueFlags & desiredFlags && (fam.queueFlags & undesiredFlags) == 0)
            return i;
            i++;
    }
    return -1;
}

imp::QueueFamilyIndices imp::GraphicsCaps::GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    indices.transferFamily = GetDesiredQueue(queueFamilyList, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    indices.graphicsFamily = GetDesiredQueue(queueFamilyList, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0);
    
    // not enough time to remember how to pass lambda to a function, so using this to find presentation queueu
    for (int i = 0; const auto& queueFamily : queueFamilyList)
    {
        // check if queue family supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
        if (presentationSupport)
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

void imp::GraphicsCaps::CollectSupportedFeatures(VkPhysicalDevice device, const std::vector<const char*>& extensionsUsed)
{
    m_MeshShadingSupported = std::find_if(extensionsUsed.begin(), extensionsUsed.end(), [](auto ex) { return strcmp(ex, VK_NV_MESH_SHADER_EXTENSION_NAME) == 0; }) != extensionsUsed.end();

    // TODO nice-to-have: find support for other stuff like bindless and nvidia nsight extensions..
}

void imp::GraphicsCaps::SetQueueFamilies(QueueFamilyIndices& fams)
{
    m_QueueFamilyIndices = fams;
}

bool imp::GraphicsCaps::IsMeshShadingSupported() const
{
    return m_MeshShadingSupported;
}

VkSurfaceFormatKHR imp::GraphicsCaps::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_SRGB,  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_SRGB || format.format == VK_FORMAT_B8G8R8A8_SRGB)
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

std::vector<const char*> imp::GraphicsCaps::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requestedExtens) const
{
    std::vector<const char*> extensionsToBeUsed;
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0)
        return extensionsToBeUsed;

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    // Renderdoc no work with nsight
    for (const auto& deviceExtension : requestedExtens)
    {
        bool hasExtension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                extensionsToBeUsed.push_back(deviceExtension);
                break;
            }
        }

        if (!hasExtension)
        {
            if (strcmp(deviceExtension, VK_NV_MESH_SHADER_EXTENSION_NAME) == 0)
            {
                printf("[VK DEVICE INIT]: Device Extension '%s' not supported! Mesh Shading will fall back to regular GPU-driven pipeline\n", deviceExtension);
            }
            // TODO NSIGHT: add nsight extension fallback options here
            else
            {
                printf("[VK DEVICE INIT]: Device Extension '%s' not supported! No fallback detected..\n", deviceExtension);
                return extensionsToBeUsed;
            }
        }
    }

    return extensionsToBeUsed;
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

uint32_t imp::MemoryProps::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags flags) const
{
    // TODO: I'm not sure if this function makes much sense
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return ~0u;
}
