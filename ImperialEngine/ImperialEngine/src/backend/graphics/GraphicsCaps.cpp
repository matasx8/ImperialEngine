#include "backend/graphics/GraphicsCaps.h"
#include "vulkan.h"
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

bool imp::GraphicsCaps::IsDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = getQueueFamilies(device);

    bool extensSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;

    if (extensSupported)
    {
        SwapChainDetails swapChainDetails = getSwapChainDetails(device);
        swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
    }

    return indices.isValid() && extensSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}
