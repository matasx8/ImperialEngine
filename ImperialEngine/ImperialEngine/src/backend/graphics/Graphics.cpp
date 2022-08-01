#include "backend/graphics/Graphics.h"
#include "frontend/Window.h"
#include <vector>
#include <stdexcept>
#include <set>
#include <cassert>
#include <extern/IMGUI/backends/imgui_impl_vulkan.h>

imp::Graphics::Graphics() : m_Settings(), m_GfxCaps(), m_ValidationLayers(), m_Window(), m_ImGUI()
{
    m_CurrentFrame = 0;
}

void imp::Graphics::Initialize(const EngineGraphicsSettings& settings, Window* window)
{
	m_Settings = settings;
    CreateInstance();
    CreateVkWindow(window);
    FindPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateCommandBufferManager();
    CreateSurfaceManager();
    CreateGarbageCollector();
    CreateImGUI();

    // create renderpass..
    renderpass = new RenderPass();
    RenderPassDesc defaultpass =
    {
        1,	// msaaCount
        1, // collor att count
        VK_FORMAT_R8G8B8A8_UNORM,
        kLoadOpClear,
        kStoreOpStore,
        VK_FORMAT_D32_SFLOAT,
        kLoadOpClear,
        kStoreOpDontCare
    };
    SurfaceDesc colorDesc =  m_Swapchain.GetSwapchainImageSurfaceDesc();
    colorDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    const SurfaceDesc depthDesc = {
        colorDesc.width,
        colorDesc.height,
        VK_FORMAT_D32_SFLOAT,
        1,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        false,
        false
    };

    std::vector<SurfaceDesc> surfaceDescriptions;
    std::vector<SurfaceDesc> resolveDescs;
    surfaceDescriptions.push_back(colorDesc);
    surfaceDescriptions.push_back(depthDesc);
    renderpass->Create(m_LogicalDevice, defaultpass, surfaceDescriptions, resolveDescs);
}

void imp::Graphics::PrototypeRenderPass()
{
    renderpass->Execute(*this);
    // should always return owned surfaces
    auto surfaces = renderpass->GiveSurfaces();
    m_SurfaceManager.ReturnSurfaces(surfaces);
}

void imp::Graphics::EndFrame()
{
    m_Swapchain.Present(m_PresentationQueue, m_CbManager.GetCommandExecSemaphores());
    m_CbManager.SignalFrameEnded();
    m_VulkanGarbageCollector.DestroySafeResources(m_LogicalDevice, m_CurrentFrame);
    m_CurrentFrame++;
}

void imp::Graphics::Destroy()
{
    vkDeviceWaitIdle(m_LogicalDevice);

    // prototyping stuff ---
    renderpass->Destroy(m_LogicalDevice);

    // prototyping stuff ---

    m_SurfaceManager.Destroy(m_LogicalDevice);
    m_VulkanGarbageCollector.DestroyAllImmediate(m_LogicalDevice);
    m_CbManager.Destroy(m_LogicalDevice);
    m_Swapchain.Destroy(m_LogicalDevice);
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
    m_GfxCaps.SetQueueFamilies(indices);

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

void imp::Graphics::CreateCommandBufferManager()
{
    m_CbManager.Initialize(m_LogicalDevice, m_GfxCaps.GetQueueFamilies(), m_Settings.swapchainImageCount);
}

void imp::Graphics::CreateSurfaceManager()
{
    MemoryProps props;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
    props.memoryProperties = memoryProperties;

    m_SurfaceManager.Initialize(m_LogicalDevice, props);
}

void imp::Graphics::CreateGarbageCollector()
{
    m_VulkanGarbageCollector.Initialize(m_Settings.swapchainImageCount);
}

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void imp::Graphics::CreateImGUI()
{
    VkDescriptorPool pool = VK_NULL_HANDLE;
    {   // temporary descriptor pool
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        auto res = vkCreateDescriptorPool(m_LogicalDevice, &pool_info, nullptr, &pool);
        check_vk_result(res);
    }

    VkRenderPass rp = VK_NULL_HANDLE;
    {
        VkAttachmentDescription attachment = {};
        attachment.format = m_Swapchain.GetSwapchainImageSurfaceDesc().format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        auto err = vkCreateRenderPass(m_LogicalDevice, &info, nullptr, &rp);
        check_vk_result(err);
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_VkInstance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_LogicalDevice;
    init_info.QueueFamily = m_GfxCaps.GetQueueFamilies().graphicsFamily;
    init_info.Queue = m_GfxQueue;
    init_info.PipelineCache = 0;
    init_info.DescriptorPool = pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = kEngineSwapchainDoubleBuffering;
    init_info.ImageCount = m_Swapchain.GetSwapchainImageCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, rp); // resume with font upload

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
