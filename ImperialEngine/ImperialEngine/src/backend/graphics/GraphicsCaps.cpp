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
