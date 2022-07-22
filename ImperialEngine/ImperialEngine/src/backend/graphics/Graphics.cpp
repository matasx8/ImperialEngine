#include "backend/graphics/Graphics.h"
#include "frontend/Window.h"
#include <vector>
#include <stdexcept>
#include <set>
#include <cassert>

imp::Graphics::Graphics() : m_Settings(), m_GfxCaps(), m_ValidationLayers(), m_Window()
{
}

void imp::Graphics::Initialize(const EngineGraphicsSettings& settings, Window* window)
{
	m_Settings = settings;
    CreateInstance();
    CreateVkWindow(window);
    FindPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
}

void imp::Graphics::Destroy()
{
    vkDeviceWaitIdle(m_LogicalDevice);

    vkDestroyDevice(m_LogicalDevice, nullptr);
    m_Window.Destroy(m_VkInstance);
    m_ValidationLayers.Destroy(m_VkInstance);
    vkDestroyInstance(m_VkInstance, nullptr);
}

void imp::Graphics::CreateInstance()
{
    m_Settings.validationLayersEnabled = m_Settings.validationLayersEnabled ? m_GfxCaps.ValidationLayersSupported() : false;
    if (m_Settings.validationLayersEnabled)
        m_Settings.requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Imperial Engine";
    appInfo.applicationVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (!CheckExtensionsSupported(m_Settings.requiredExtensions))
        throw std::runtime_error("Fatal error, some extension is not supported.");

    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_Settings.requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = m_Settings.requiredExtensions.data();
    createInfo.enabledLayerCount = m_Settings.validationLayersEnabled ? 1 : 0;
    createInfo.ppEnabledLayerNames = &m_GfxCaps.validationLayerName;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (m_Settings.validationLayersEnabled)
    {
        debugCreateInfo = ValidationLayers::CreateDebugMessengerCreateInfo(m_Settings);
        createInfo.pNext = &debugCreateInfo;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VkInstance);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Vulkan instance");

    printf("Vulkan Instance created successfully\n");
    if (m_Settings.validationLayersEnabled)
    {
        m_ValidationLayers.EnableCallback(m_VkInstance, m_Settings);
        printf("Vulkan Validation layers enabled!\n");
    }
    else
        printf("Vulkan Validation layers disabled!\n");
}

void imp::Graphics::FindPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Can't find GPUs that support Vulkan instance");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

    bool foundSuitableDevice = false;
    for (const auto& device : devices)
    {
        if (m_GfxCaps.IsDeviceSuitable(device, m_Window.GetWindowSurface()) && m_GfxCaps.CheckDeviceExtensionSupport(device, m_Settings.requiredDeviceExtensions))
        {
            m_PhysicalDevice = device;
            foundSuitableDevice = true;
            break;
        }
    }

    if (!foundSuitableDevice)
        throw std::runtime_error("Vulkan Error! Didn't find a suitable physical device!");
}

void imp::Graphics::CreateLogicalDevice()
{
    QueueFamilyIndices indices = m_GfxCaps.GetQueueFamilies(m_PhysicalDevice, m_Window.GetWindowSurface());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // index of the graphics family to create a queue from
        queueCreateInfo.queueCount = 1;
        float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority; // vulkan needs to know multiple queues
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // TODO: Nsight...
    VkDeviceDiagnosticsConfigCreateInfoNV dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
    dci.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |      // Enable tracking of resources.
        VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |  // Capture call stacks for all draw calls, compute dispatches, and resource copies.
        VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV;

    //information to create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); //number of queue create infos
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_Settings.requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_Settings.requiredDeviceExtensions.data();
    deviceCreateInfo.pNext = &dci;

    VkPhysicalDeviceFeatures devicefeatures = {};
    devicefeatures.samplerAnisotropy = VK_TRUE;

    deviceCreateInfo.pEnabledFeatures = &devicefeatures;

    VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a logical device!");
    printf("Vulkan Logical device successfully created!\n"); // TODO: log more stuff, like extensions used and etc.

    // TODO: Make system for multiple queues when needed
    vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily, 0, &m_GfxQueue);
    vkGetDeviceQueue(m_LogicalDevice, indices.presentationFamily, 0, &m_PresentationQueue);
}

void imp::Graphics::CreateVkWindow(Window* window)
{
    assert(window);
    m_Window = VkWindow(*window);
    m_Window.CreateWindowSurface(m_VkInstance);
}

void imp::Graphics::CreateSwapchain()
{
    m_Swapchain.Create(m_PhysicalDevice, m_LogicalDevice, m_Window.GetWindowSurface(), m_Settings, m_GfxCaps.GetPhysicalDeviceSurfaceCaps(), m_Window.GetExtent());
}

bool imp::Graphics::CheckExtensionsSupported(std::vector<const char*> extensions)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensionsSupported(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsSupported.data());

    for (const auto& checkExtension : extensions)
    {
        bool hasExtension = false;
        for (const auto& extension : extensionsSupported)
        {
            if (strcmp(checkExtension, extension.extensionName))
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
    }
    return true;
}
