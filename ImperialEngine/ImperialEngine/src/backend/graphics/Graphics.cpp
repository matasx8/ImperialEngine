#include "backend/graphics/RenderPass/RenderPassFactorySimple.h"
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

namespace imp
{
    Graphics::Graphics() :
        m_Settings(),
        m_GfxCaps(),
        m_ValidationLayers(),
        m_VkInstance(),
        m_PhysicalDevice(),
        m_LogicalDevice(),
        m_GfxQueue(),
        m_PresentationQueue(),
        m_Swapchain(m_SemaphorePool),
        m_CurrentFrame(0ul),
        m_VulkanGarbageCollector(),
        m_CbManager(m_SemaphorePool, m_FencePool, m_SyncTimer),
        m_SurfaceManager(),
        m_ShaderManager(),
        m_PipelineManager(),
        m_RenderPassManager(&m_VulkanGarbageCollector),
        m_Timer(),
        m_OldTimer(),
        m_SyncTimer(),
        m_OldSyncTimer(),
        m_SemaphorePool(SemaphoreFactory()),
        m_FencePool(FenceFactory()),
        m_Window(),
        m_MemoryManager(),
        m_DeviceMemoryProps(),
        m_VertexBuffer(),
        m_IndexBuffer(),
        m_MeshBuffer(),
        m_DrawBuffer(),
        m_NumDraws(),
        m_GlobalBuffers(),
        m_DescriptorSets(),
        m_VertexBuffers(),
        m_DrawData(),
        m_CameraData(),
        renderpassgui()
    {
    }

    void Graphics::Initialize(const EngineGraphicsSettings& settings, Window* window)
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
        m_ShaderManager.Initialize(m_LogicalDevice, m_MemoryManager, m_Settings, m_DeviceMemoryProps, m_DrawBuffer);

        // Until we haven't made custom vulkan backend for imgui we can't fully have dynamic RenderPassGenerator
        // since CreateImGUI needs a renderpass
        // so as a temporary workaround we must create the imguiPass upfront and save it
        // this will work while the imgui pass remains completely static
        auto temp = CameraData();
        renderpassgui = m_RenderPassManager.GetImGUIPass(m_LogicalDevice, temp, m_Swapchain);
        CreateImGUI();
    }

    // TODO ST-BUG:
    // This doesn't work when going in ST because I'm using this bool as a hack to
    // upload the first data to DrawBuffer. When I remove this ST should work.
    static bool FirstFrame = true;

    void Graphics::DispatchUpdateDrawCommands()
    {
        // Update the new global draw count
        m_NumDraws = GetDrawDataStagingBuffer().size();
        const auto dispatchCount = (m_NumDraws + 31) / 32;

        // TODO nice-to-have: make the interface for getting shaders better. At least make
        // shader manager return the configs immediately
        const auto updateDrawCS = m_ShaderManager.GetShader("drawGen.comp");
        ComputePipelineConfig config = { updateDrawCS.GetShaderModule(), m_ShaderManager.GetDescriptorSetLayout(),m_ShaderManager.GetComputeDescriptorSetLayout()};
        const auto updateDrawsProgram = m_PipelineManager.GetComputePipeline(config);
        const auto dset1 = m_ShaderManager.GetDescriptorSet(m_Swapchain.GetFrameClock());
        const auto dset2 = m_ShaderManager.GetComputeDescriptorSet(m_Swapchain.GetFrameClock());
        std::array<VkDescriptorSet, 2> dsets = { dset1, dset2 };

        // TODO: make RAII?
        CommandBuffer cb = m_CbManager.AquireCommandBuffer(m_LogicalDevice);
        cb.Begin();
        vkCmdBindPipeline(cb.cmb, VK_PIPELINE_BIND_POINT_COMPUTE, updateDrawsProgram.GetPipeline());
        vkCmdBindDescriptorSets(cb.cmb, VK_PIPELINE_BIND_POINT_COMPUTE, updateDrawsProgram.GetPipelineLayout(), 0, dsets.size(), dsets.data(), 0, nullptr);
        vkCmdDispatch(cb.cmb, (m_NumDraws + 31) / 32, 1, 1);

        // need to make sure memory write is visible
        VkBufferMemoryBarrier bufferMemBar = {};
        bufferMemBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferMemBar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bufferMemBar.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        bufferMemBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferMemBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferMemBar.buffer = m_DrawBuffer.GetBuffer();
        bufferMemBar.offset = 0;
        bufferMemBar.size = VK_WHOLE_SIZE;

        // flush stages until CS is done
        // suspend until DRAW INDRICET
        vkCmdPipelineBarrier(cb.cmb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &bufferMemBar, 0, nullptr);
        cb.End();

        // Ideally would batch this to some transfer / compute queue that happens at the start of the frame.
        // But for now just submit to avoid having to many submit to queue commands
        m_CbManager.SubmitInternal(cb);
        m_ShaderManager.GetDrawDataBuffer(m_Swapchain.GetFrameClock()).GiveFence(m_CbManager.GetCurrentFence()); // get fence from CB
    }

    void Graphics::StartFrame()
    {
        m_ShaderManager.UpdateDrawData(m_LogicalDevice, m_Swapchain.GetFrameClock(), m_DrawData);
    }

    void Graphics::RenderCameras()
    {
        for (const auto& camera : m_CameraData)
        {
            GlobalData data;
            data.ViewProjection = camera.Projection * camera.View;
            m_ShaderManager.UpdateGlobalData(m_LogicalDevice, m_Swapchain.GetFrameClock(), data);

            auto& renderPasses = m_RenderPassManager.GetRenderPasses(m_LogicalDevice, camera, m_Swapchain);
            for (auto& rp : renderPasses)
            {
                rp->Execute(*this, camera);
                auto surfaces = rp->GiveSurfaces();
                m_SurfaceManager.ReturnSurfaces(surfaces, m_Swapchain);
            }
        }
    }

    void Graphics::RenderImGUI()
    {
        renderpassgui->Execute(*this, m_CameraData[0]);
        auto surfaces = renderpassgui->GiveSurfaces();
        m_SurfaceManager.ReturnSurfaces(surfaces, m_Swapchain);
    }

    void Graphics::EndFrame()
    {
        m_CbManager.SubmitToQueue(m_GfxQueue, m_LogicalDevice, kSubmitSynchForPresent, m_CurrentFrame);

        m_Swapchain.Present(m_PresentationQueue, m_CbManager.GetCommandExecSemaphores());
        m_CbManager.SignalFrameEnded();
        m_SurfaceManager.SignalFrameEnded();
        m_VulkanGarbageCollector.DestroySafeResources(m_LogicalDevice, m_CurrentFrame);
        m_CurrentFrame++;
        m_Timer.frameWorkTime.stop();
    }

    void Graphics::CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData)
    {
        const uint32_t numCbs = 1;
        auto cbs = m_CbManager.AquireCommandBuffers(m_LogicalDevice, numCbs);
        auto& cb = cbs[0];

        cb.Begin();

        // TODO: this can be abstracted to not differentiate between indices and vertices

        uint32_t vtxAllocSize = 0;
        uint32_t idxAllocSize = 0;
        std::vector<Vertex> verts;
        std::vector<uint32_t> idxs;
        const uint32_t vertBufferOffset = m_VertexBuffer.GetOffset() / sizeof(Vertex);
        const uint32_t indBufferOffset = m_IndexBuffer.GetOffset() / sizeof(uint32_t);
        for (const auto& req : meshCreationData)
        {
            // These subbuffers will be used to index and offset into the one bound Vertex and Index buffer
            VulkanSubBuffer vtxSub = VulkanSubBuffer(verts.size() + vertBufferOffset, static_cast<uint32_t>(req.vertices.size()));
            VulkanSubBuffer idxSub = VulkanSubBuffer(idxs.size() + indBufferOffset, static_cast<uint32_t>(req.indices.size()));
            verts.insert(verts.end(), req.vertices.begin(), req.vertices.end());
            idxs.insert(idxs.end(), req.indices.begin(), req.indices.end());
            vtxAllocSize += static_cast<uint32_t>(req.vertices.size() * sizeof(Vertex));
            idxAllocSize += static_cast<uint32_t>(req.indices.size() * sizeof(uint32_t));

            m_VertexBuffers[req.id] = { idxSub, vtxSub };
        }

        const auto usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const auto memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        // upload vertices
        UploadVulkanBuffer(usageFlags, memoryFlags, m_VertexBuffer, cb, vtxAllocSize, verts.data());

        // upload indices
        UploadVulkanBuffer(usageFlags, memoryFlags, m_IndexBuffer, cb, idxAllocSize, idxs.data());

        cb.End();

        m_CbManager.SubmitInternal(cb);
        auto synchs = m_CbManager.SubmitToQueue(m_GfxQueue, m_LogicalDevice, kSubmitDontCare, m_CurrentFrame);
        m_VertexBuffer.GiveSemaphore(synchs.semaphore.semaphore);
        m_IndexBuffer.GiveSemaphore(synchs.semaphore.semaphore);
    }

    void Graphics::CreateAndUploadMaterials(const std::vector<CmdRsc::MaterialCreationRequest>& materialCreationData)
    {
        for (const auto& req : materialCreationData)
        m_ShaderManager.CreateVulkanShaderSet(m_LogicalDevice, req);
    }

    void Graphics::CreateComputePrograms(const std::vector<CmdRsc::ComputeProgramCreationRequest>& computeProgramRequests)
    {
        for (const auto& req : computeProgramRequests)
            m_ShaderManager.CreateComputePrograms(m_LogicalDevice, m_PipelineManager, req);
    }

    IGPUBuffer& Graphics::GetDrawDataStagingBuffer()
    {
        auto& drawDataBuffer = m_ShaderManager.GetDrawDataBuffer(m_Swapchain.GetFrameClock());

        // Dont wait on fence that might've been reset already, since cb manager controls those fences.
        // potential design flaw to consider fixing later
        if(m_CurrentFrame - drawDataBuffer.GetLastUsed() >= m_Settings.swapchainImageCount)
            drawDataBuffer.WaitUntilNotUsedByGPU(m_LogicalDevice, m_FencePool);

        return drawDataBuffer;
    }

    const Comp::IndexedVertexBuffers& Graphics::GetMeshData(uint32_t index) const
    {
        return m_VertexBuffers.at(index);
    }

    EngineGraphicsSettings& Graphics::GetGraphicsSettings()
    {
        return m_Settings;
    }

    void Graphics::Destroy()
    {
        vkDeviceWaitIdle(m_LogicalDevice);

        // prototyping stuff ---
        renderpassgui->Destroy(m_LogicalDevice);
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

    void Graphics::CreateInstance()
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

    void Graphics::FindPhysicalDevice()
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

    void Graphics::CreateLogicalDevice()
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
        // deviceCreateInfo.pNext = &dci;  // not using nsight so far

        VkPhysicalDeviceFeatures devicefeatures = {};
        devicefeatures.samplerAnisotropy = VK_TRUE;


        // deviceCreateInfo.pEnabledFeatures = &devicefeatures;

         // indexing ---
        VkPhysicalDeviceShaderDrawParametersFeatures drawParamsFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES, nullptr, VK_TRUE };
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
        VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        physical_features2.features.samplerAnisotropy = VK_TRUE;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &physical_features2);
        physical_features2.features.multiDrawIndirect = true;
        bool bindless_supported = indexing_features.descriptorBindingPartiallyBound && indexing_features.runtimeDescriptorArray;
        indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
        indexing_features.runtimeDescriptorArray = VK_TRUE;
        indexing_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        indexing_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
        indexing_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        indexing_features.pNext = &drawParamsFeature;
        physical_features2.pNext = &indexing_features;
        deviceCreateInfo.pNext = &physical_features2;


        VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create a logical device!");
        printf("Vulkan Logical device successfully created!\n"); // TODO: log more stuff, like extensions used and etc.

        // TODO: Make system for multiple queues when needed
        vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily, 0, &m_GfxQueue);
        vkGetDeviceQueue(m_LogicalDevice, indices.presentationFamily, 0, &m_PresentationQueue);
    }

    void Graphics::CreateVkWindow(Window* window)
    {
        assert(window);
        m_Window = VkWindow(*window);
        m_Window.CreateWindowSurface(m_VkInstance);
    }

    void Graphics::CreateSwapchain()
    {
        m_Swapchain.Create(m_PhysicalDevice, m_LogicalDevice, m_Window.GetWindowSurface(), m_Settings, m_GfxCaps.GetPhysicalDeviceSurfaceCaps(), m_Window.GetExtent());
    }

    void Graphics::CreateCommandBufferManager()
    {
        m_CbManager.Initialize(m_LogicalDevice, m_GfxCaps.GetQueueFamilies(), m_Settings.swapchainImageCount);
    }

    void Graphics::CreateSurfaceManager()
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
        m_DeviceMemoryProps.memoryProperties = memoryProperties;

        // TODO: in vulkan memory manager I approached this by giving m_DeviceMemoryProps to where it's needed. Maybe that's better
        // and probably it is. This is needed here because we allocate an image but without our memory manager. So would be good to use it later for all allocations
        m_SurfaceManager.Initialize(m_LogicalDevice, m_DeviceMemoryProps);
    }

    void Graphics::CreateGarbageCollector()
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

    void Graphics::CreateImGUI()
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
        m_CbManager.SubmitInternal(cbs[0]);
        auto synchs = m_CbManager.SubmitToQueue(m_GfxQueue, m_LogicalDevice, kSubmitDontCare, m_CurrentFrame);

        // Since we're completely discarding this semaphore nothing will wait for it, can throw out
        m_VulkanGarbageCollector.AddGarbageResource(std::make_shared<Semaphore>(synchs.semaphore));

        // TODO: dont wait
        auto err = vkDeviceWaitIdle(m_LogicalDevice);
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void Graphics::CreateVulkanMemoryManager()
    {
        // m_MemoryManager.Initialize();
    }

    void Graphics::CreateRenderPassGenerator()
    {
        std::unique_ptr<RenderPassFactory> RpFactory = std::make_unique<RenderPassFactorySimple>();
        m_RenderPassManager.SetRenderPassFactory(std::move(RpFactory));
    }

    void Graphics::InitializeVulkanMemory()
    {
        static constexpr VkDeviceSize allocSize = 4 * 1024 * 1024;
        static constexpr VkDeviceSize drawAllocSize = (kMaxDrawCount + 31) * sizeof(VkDrawIndexedIndirectCommand);
        m_VertexBuffer  = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_IndexBuffer   = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_MeshBuffer    = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_DrawBuffer    = m_MemoryManager.GetBuffer(m_LogicalDevice, drawAllocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
    }

    VulkanBuffer Graphics::UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkBufferUsageFlags dstUsageFlags, VkMemoryPropertyFlags memoryFlags, VkMemoryPropertyFlags dstMemoryFlags, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload)
    {
        auto uploadedBuffer = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, dstUsageFlags, dstMemoryFlags, m_DeviceMemoryProps);
        UploadVulkanBuffer(usageFlags, memoryFlags, uploadedBuffer, cb, allocSize, dataToUpload);
        return uploadedBuffer;
    }

    void Graphics::UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VulkanBuffer& dst, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload)
    {
        // TODO: later change this so it returns buffer that's on scratch memory
        auto stagingBuffer = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, usageFlags, memoryFlags, m_DeviceMemoryProps);
        // then as a vulkan resource we can override the destruction so it doesn't try to deallocate a sub-buffer and just notifies the scratch there's a spot open
        stagingBuffer.UpdateLastUsed(m_CurrentFrame);
        dst.UpdateLastUsed(m_CurrentFrame);

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

    void Graphics::CopyVulkanBuffer(const VulkanBuffer& src, VulkanBuffer& dst, const CommandBuffer& cb)
    {
        assert(src.GetSize());
        assert(dst.GetSize() >= src.GetSize());

        VkBufferCopy bufferCopyRegion;
        bufferCopyRegion.srcOffset = 0;
        bufferCopyRegion.dstOffset = dst.GetOffset();;
        bufferCopyRegion.size = src.GetSize();

        vkCmdCopyBuffer(cb.cmb, src.GetBuffer(), dst.GetBuffer(), 1, &bufferCopyRegion);

        dst.RegisterNewUpload(src.GetSize());
    }

    const Pipeline& Graphics::EnsurePipeline(VkCommandBuffer cb, const RenderPass& rp)
    {
        // Compare old material and new one
        // if true: return
        // else: get pipeline from pipeline manager
        // Material should store precalculates hash of shader name
        PipelineConfig tempConfig;
        if (m_Settings.renderMode == kEngineRenderModeTraditional)
            tempConfig.vertModule = m_ShaderManager.GetShader("basic.vert").GetShaderModule();
        else
            tempConfig.vertModule = m_ShaderManager.GetShader("basic.ind.vert").GetShaderModule();
        tempConfig.fragModule = m_ShaderManager.GetShader("basic.frag").GetShaderModule();

        tempConfig.descriptorSetLayout = m_ShaderManager.GetDescriptorSetLayout();

        const auto& pipeline = m_PipelineManager.GetOrCreatePipeline(m_LogicalDevice, rp, tempConfig);
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
        return pipeline;
    }

    void Graphics::PushConstants(VkCommandBuffer cb, const void* data, uint32_t size, VkPipelineLayout pipeLayout) const
    {
        assert(data);
        assert(size);
        vkCmdPushConstants(cb, pipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
    }

    void Graphics::DrawIndexed(VkCommandBuffer cb, uint32_t indexCount) const
    {
        vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
    }

    bool Graphics::CheckExtensionsSupported(std::vector<const char*> extensions)
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
}