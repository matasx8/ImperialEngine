#include "backend/graphics/RenderPassGeneratorSimple.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/Graphics.h"
#include "backend/VulkanBuffer.h"
#include "frontend/Components/Components.h"
#include "frontend/Window.h"
#include <vector>
#include <stdexcept>
#include <set>
#include <cassert>
#include <extern/IMGUI/backends/imgui_impl_vulkan.h>
#include <numeric>

imp::Graphics::Graphics() : m_Settings(), m_GfxCaps(), m_ValidationLayers(), m_Window(), m_PipelineManager(), m_MemoryManager()
{
    // TODO: init all variables
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
    CreateRenderPassGenerator();

    InitializeVulkanMemory();

    // Until we haven't made custom vulkan backend for imgui we can't fully have dynamic RenderPassGenerator
    // since CreateImGUI needs a renderpass
    // so as a temporary workaround we must create the imguiPass upfront and save it
    // this will work while the imgui pass remains completely static
    auto temp = CameraData();
    renderpassgui = static_cast<RenderPassGeneratorSimple*>(m_RenderPassManager)->GenerateForImGUIPass(m_LogicalDevice, temp, m_Swapchain);
    CreateImGUI();
}

void imp::Graphics::RenderCameras()
{
    // generate container of renderpasses
    // first can be a simple vector of renderpasses but in the future can be a render graph
    // iterate and execute

    for (const auto& camera : m_CameraData)
    {
        // GenerateRenderPasses must ensure that sequentially executed render passes will have correct sequence and inputs
        auto rps = m_RenderPassManager->GenerateRenderPasses(m_LogicalDevice, camera, m_Swapchain);
        for (auto& rp : rps)
        {
            rp->Execute(*this, camera);
            auto surfaces = rp->GiveSurfaces();
            m_SurfaceManager.ReturnSurfaces(surfaces);
        }
    }
    //RenderImGUI();
}

void imp::Graphics::RenderImGUI()
{
    renderpassgui->Execute(*this, m_CameraData[0]);
    auto surfaces = renderpassgui->GiveSurfaces();
    m_SurfaceManager.ReturnSurfaces(surfaces);
}

void imp::Graphics::EndFrame()
{
    m_Swapchain.Present(m_PresentationQueue, m_CbManager.GetCommandExecSemaphores());
    m_CbManager.SignalFrameEnded();
    m_VulkanGarbageCollector.DestroySafeResources(m_LogicalDevice, m_CurrentFrame);
    m_CurrentFrame++;
}

void imp::Graphics::CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData)
{
    const uint32_t numCbs = 1;
    auto cbs = m_CbManager.AquireCommandBuffers(m_LogicalDevice, numCbs);
    auto& cb = cbs[0];

    cb.Begin();


    //auto allocSize = std::accumulate(meshCreationData.begin(), meshCreationData.end(), 0u, [](const auto& a, const auto& b) { return a + b.vertices.size(); });//.vertices.size() * sizeof(Vertex);
    //UploadVulkanBuffer(usageFlags, dstUsageFlags, memoryFlags, dstMemoryFlags, cb, allocSize, req.vertices.data());

    // TODO: this can be abstracted to not differentiate between indices and vertices
    uint32_t vtxAllocSize = 0;
    uint32_t idxAllocSize = 0;
    std::vector<Vertex> verts;
    std::vector<uint32_t> idxs;
    for (const auto& req : meshCreationData)
    {
        VulkanSubBuffer vtxSub = VulkanSubBuffer(verts.size(), req.vertices.size());
        VulkanSubBuffer idxSub = VulkanSubBuffer(idxs.size(), req.indices.size());
        verts.insert(verts.end(), req.vertices.begin(), req.vertices.end());
        idxs.insert(idxs.end(), req.indices.begin(), req.indices.end());
        vtxAllocSize += req.vertices.size() * sizeof(Vertex);
        idxAllocSize += req.indices.size() * sizeof(uint32_t);

        m_VertexBuffers[req.id] = { vtxSub, idxSub };
    }

    const auto usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    const auto memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // upload vertices
    UploadVulkanBuffer(usageFlags, memoryFlags, m_VertexBuffer, cb, vtxAllocSize, verts.data());

    // upload indices
    UploadVulkanBuffer(usageFlags, memoryFlags, m_IndexBuffer, cb, idxAllocSize, idxs.data());

    cb.End();
    
    auto synchs = m_CbManager.Submit(m_GfxQueue, m_LogicalDevice, cbs, {});
    m_VertexBuffer.GiveSemaphore(synchs.semaphore);
    m_IndexBuffer.GiveSemaphore(synchs.semaphore);
}

void imp::Graphics::CreateAndUploadMaterials(const std::vector<CmdRsc::MaterialCreationRequest>& materialCreationData)
{
    for (const auto& req : materialCreationData)
    {
        m_ShaderManager.CreateVulkanShaderSet(m_LogicalDevice, req);
    }
}

void imp::Graphics::Destroy()
{
    vkDeviceWaitIdle(m_LogicalDevice);

    // prototyping stuff ---
    renderpassgui->Destroy(m_LogicalDevice);
    // prototyping stuff ---

    delete m_RenderPassManager;
    m_SurfaceManager.Destroy(m_LogicalDevice);
    m_VulkanGarbageCollector.DestroyAllImmediate(m_LogicalDevice);
    m_CbManager.Destroy(m_LogicalDevice);
    m_Swapchain.Destroy(m_LogicalDevice);
    vkDestroyDevice(m_LogicalDevice, nullptr);
    m_Window.Destroy(m_VkInstance);
    m_ValidationLayers.Destroy(m_VkInstance);
    vkDestroyInstance(m_VkInstance, nullptr);
}

VkSemaphore imp::Graphics::GetSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore sem;
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sem);
    return sem;
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
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
    m_DeviceMemoryProps.memoryProperties = memoryProperties;

    // TODO: in vulkan memory manager I approached this by giving m_DeviceMemoryProps to where it's needed. Maybe that's better
    // and probably it is. This is needed here because we allocate an image but without our memory manager. So would be good to use it later for all allocations
    m_SurfaceManager.Initialize(m_LogicalDevice, m_DeviceMemoryProps);
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
    assert(renderpassgui);
    ImGui_ImplVulkan_Init(&init_info, renderpassgui->GetVkRenderPass());

    auto cbs = m_CbManager.AquireCommandBuffers(m_LogicalDevice, 1);
    cbs[0].Begin();
    ImGui_ImplVulkan_CreateFontsTexture(cbs[0].cmb);
    cbs[0].End();
    m_CbManager.Submit(m_GfxQueue, m_LogicalDevice, cbs, {});

    // TODO: dont wait
    auto err = vkDeviceWaitIdle(m_LogicalDevice);
    check_vk_result(err);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void imp::Graphics::CreateVulkanMemoryManager()
{
    m_MemoryManager.Initialize();
}

void imp::Graphics::CreateRenderPassGenerator()
{
    m_RenderPassManager = new RenderPassGeneratorSimple();
}

void imp::Graphics::InitializeVulkanMemory()
{
    static constexpr VkDeviceSize allocSize = 128 * 1024 * 1024;
    m_VertexBuffer  = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
    m_IndexBuffer   = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
    m_MeshBuffer    = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
}

imp::VulkanBuffer imp::Graphics::UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkBufferUsageFlags dstUsageFlags, VkMemoryPropertyFlags memoryFlags, VkMemoryPropertyFlags dstMemoryFlags, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload)
{
    // TODO: later change this so it returns buffer that's on scratch memory
    auto stagingBuffer = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, usageFlags, memoryFlags, m_DeviceMemoryProps);
    // then as a vulkan resource we can override the destruction so it doesn't try to deallocate a sub-buffer and just notifies the scratch there's a spot open
    stagingBuffer.UpdateLastUsed(m_CurrentFrame);
    // continue here.. going off to create storage buffers
    auto uploadedBuffer = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, dstUsageFlags, dstMemoryFlags, m_DeviceMemoryProps);

    // copy cpu data to host visible buffer
    void* data;
    const VkMemoryMapFlags flags = 0;   // don't exist yet
    const VkDeviceSize offset = 0;      // so far we have whole buffers
    const auto res = vkMapMemory(m_LogicalDevice, stagingBuffer.GetMemory(), offset, stagingBuffer.GetSize(), flags, &data);
    assert(res == VK_SUCCESS);
    memcpy(data, dataToUpload, stagingBuffer.GetSize());
    vkUnmapMemory(m_LogicalDevice, stagingBuffer.GetMemory());

    // do host visible buffer to device local buffer
    CopyVulkanBuffer(stagingBuffer, uploadedBuffer, cb);

    m_VulkanGarbageCollector.AddGarbageResource(std::make_shared<VulkanBuffer>(stagingBuffer));

    return uploadedBuffer;
}

void imp::Graphics::UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VulkanBuffer& dst, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload)
{
    // TODO: "Calling a second time won't work since it will overwrite the dst buffer"
    // 
    // TODO: later change this so it returns buffer that's on scratch memory
    auto stagingBuffer = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, usageFlags, memoryFlags, m_DeviceMemoryProps);
    // then as a vulkan resource we can override the destruction so it doesn't try to deallocate a sub-buffer and just notifies the scratch there's a spot open
    stagingBuffer.UpdateLastUsed(m_CurrentFrame);

    // copy cpu data to host visible buffer
    void* data;
    const VkMemoryMapFlags flags = 0;   // don't exist yet
    const VkDeviceSize offset = 0;      // so far we have whole buffers
    const auto res = vkMapMemory(m_LogicalDevice, stagingBuffer.GetMemory(), offset, stagingBuffer.GetSize(), flags, &data);
    assert(res == VK_SUCCESS);
    memcpy(data, dataToUpload, stagingBuffer.GetSize());
    vkUnmapMemory(m_LogicalDevice, stagingBuffer.GetMemory());

    // do host visible buffer to device local buffer
    CopyVulkanBuffer(stagingBuffer, dst, cb);

    m_VulkanGarbageCollector.AddGarbageResource(std::make_shared<VulkanBuffer>(stagingBuffer));
}

void imp::Graphics::CopyVulkanBuffer(const VulkanBuffer& src, VulkanBuffer& dst, const CommandBuffer& cb)
{
    assert(src.GetSize());
    assert(dst.GetSize() >= src.GetSize());
    VkBufferCopy bufferCopyRegion;
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = src.GetSize();

    vkCmdCopyBuffer(cb.cmb, src.GetBuffer(), dst.GetBuffer(), 1, &bufferCopyRegion);
}

const imp::Pipeline& imp::Graphics::EnsurePipeline(VkCommandBuffer cb, const RenderPassBase& rp)
{
    // Compare old material and new one
    // if true: return
    // else: get pipeline from pipeline manager
    // Material should store precalculates hash of shader name
    PipelineConfig tempConfig;
    tempConfig.vertModule = m_ShaderManager.GetShader("basic.vert").GetShaderModule();
    tempConfig.fragModule = m_ShaderManager.GetShader("basic.frag").GetShaderModule();

    const auto& pipeline = m_PipelineManager.GetOrCreatePipeline(m_LogicalDevice, rp, tempConfig);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
    return pipeline;
}

void imp::Graphics::PushConstants(VkCommandBuffer cb, const void* data, uint32_t size, VkPipelineLayout pipeLayout) const
{
    assert(data);
    assert(size);
    vkCmdPushConstants(cb, pipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
}

uint32_t imp::Graphics::BindMesh(VkCommandBuffer cb, uint32_t vtxBufferId) const
{
    //const auto indexedVertexBufferIt = m_VertexBuffers.find(vtxBufferId);
    //assert(indexedVertexBufferIt != m_VertexBuffers.end());
    //const auto& vertexBuffer = indexedVertexBufferIt->second.vertices.GetBuffer();
    //const auto& indexBuffer = indexedVertexBufferIt->second.indices.GetBuffer();
    //VkDeviceSize offsets[] = { 0 };

    //vkCmdBindVertexBuffers(cb, 0, 1, &vertexBuffer, offsets);
    //vkCmdBindIndexBuffer(cb, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    //return indexedVertexBufferIt->second.indices.GetSize() / sizeof(uint32_t);
    return 0;
}

void imp::Graphics::DrawIndexed(VkCommandBuffer cb, uint32_t indexCount) const
{
    vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
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
