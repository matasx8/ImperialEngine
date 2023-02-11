#include "backend/graphics/RenderPass/RenderPassFactorySimple.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/Graphics.h"
#include "backend/VulkanBuffer.h"
#include "frontend/Components/Components.h"
#include "frontend/Window.h"
#include "Utils/GfxUtilities.h"
#include <vector>
#include <stdexcept>
#include <set>
#include <cassert>
#include <extern/IMGUI/backends/imgui_impl_vulkan.h>
#include <numeric>
#include <GLM/gtx/transform.hpp>

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
        m_TransferQueue(),
        m_PresentationQueue(),
        m_Swapchain(m_SemaphorePool),
        m_CurrentFrame(),
        m_VulkanGarbageCollector(),
        m_CbManager(m_SemaphorePool, m_FencePool, m_SyncTimer),
        m_TransferCbManager(m_SemaphorePool, m_FencePool, m_SyncTimer),
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
        m_StagingDrawBuffer(),
        m_BoundingVolumeBuffer(),
        m_NumDraws(),
        m_GlobalBuffers(),
        m_DescriptorSets(),
        m_VertexBuffers(),
        m_DrawData(),
        m_CameraData(),
        m_DelayTransferOperation(),
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

    void Graphics::DoTransfers(bool releaseAll)
    {
        // release resources
        std::vector<VkBufferMemoryBarrier> bmbs;
        VkAccessFlags dstAccess = 0; // dstAccess is ignored for Q ownership transfers
        uint32_t tf = m_GfxCaps.GetQueueFamilies().transferFamily;
        uint32_t gf = m_GfxCaps.GetQueueFamilies().graphicsFamily;
        
        //auto& ddb = m_ShaderManager.GetDrawDataBuffers(m_Swapchain.GetFrameClock());
        //const auto bmb_ddb = utils::CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, dstAccess, tf, gf, ddb.GetBuffer());
        //bmbs.push_back(bmb_ddb);

        if (releaseAll) // also need to release draw command buffer that might not get updated every time
        {
            auto& dcb = m_ShaderManager.GetDrawCommandBuffer();
            const auto bmb_dcb = utils::CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, dstAccess, tf, gf, dcb.GetBuffer());
            bmbs.push_back(bmb_dcb);
        }

        auto& cb = m_TransferCbManager.GetCurrentCB(m_LogicalDevice);
        utils::InsertBufferBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, bmbs.data(), bmbs.size());
        
        cb.End();
        m_TransferCbManager.ReturnCommandBufferToPool(cb, 0, m_TransferCbManager.GetCurrentFence().fence);

        // Resources used in a Queue should be marked with a semaphore
        // So when next queue uses these resources it will take its semaphore and wait on it.
        m_TransferCbManager.SubmitToTransferQueue(m_TransferQueue, m_LogicalDevice, m_CurrentFrame);
    }

    // TODO ST-BUG:
    // This doesn't work when going in ST because I'm using this bool as a hack to
    // upload the first data to DrawBuffer. When I remove this ST should work.
    static bool FirstFrame = true;

    void Graphics::UpdateDrawCommands()
    {
        // Update the new global draw count
        auto& staging = m_StagingDrawBuffer[m_Swapchain.GetFrameClock()];
        auto& dst = m_ShaderManager.GetDrawCommandBuffer();

        // give instructions for CB to wait on previous dst semaphore
        m_TransferCbManager.AddQueueDependencies(staging.GetTimeline());
        m_TransferCbManager.AddQueueDependencies(dst.GetTimeline());

        // first time used in tranfer queue, mark it
        staging.MarkUsedInQueue();
        dst.MarkUsedInQueue();

        // no need to aquire ownership since we don't care about contents

        m_NumDraws = staging.size();

        CommandBuffer& cb = m_TransferCbManager.GetCurrentCB(m_LogicalDevice);

        VkBufferCopy copy = {};
        copy.size = m_NumDraws * sizeof(IndirectDrawCmd);

        vkCmdCopyBuffer(cb.cmb, staging.GetBuffer(), dst.GetBuffer(), 1, &copy);

        assert(m_DelayTransferOperation);
        DoTransfers(true);
    }

    void Graphics::GraphicsQueueStart()
    {
 
    }

    void Graphics::Cull()
    {
        const auto dispatchCount = (m_NumDraws + 31) / 32;

        // TODO nice-to-have: make the interface for getting shaders better. At least make
        // shader manager return the configs immediately
        const auto updateDrawCS = m_ShaderManager.GetShader("drawGen.comp");
        ComputePipelineConfig config = { updateDrawCS.GetShaderModule(), m_ShaderManager.GetDescriptorSetLayout(),m_ShaderManager.GetComputeDescriptorSetLayout() };
        const auto updateDrawsProgram = m_PipelineManager.GetComputePipeline(config);
        const auto dset1 = m_ShaderManager.GetDescriptorSet(m_Swapchain.GetFrameClock());
        const auto dset2 = m_ShaderManager.GetComputeDescriptorSet(m_Swapchain.GetFrameClock());

        std::array<VkDescriptorSet, 2> dsets = { dset1, dset2 };

        glm::mat4 vp = glm::transpose(m_CameraData.front().Projection * m_CameraData.front().View);

        struct Pushs
        {
            glm::vec4 frustum[6];
            uint32_t numDraws;
        } push;
        push.frustum[0] = glm::normalize(vp[3] + vp[0]);
        push.frustum[1] = glm::normalize(vp[3] - vp[0]);
        push.frustum[2] = glm::normalize(vp[3] + vp[1]);
        push.frustum[3] = glm::normalize(vp[3] - vp[1]);
        push.frustum[4] = glm::normalize(vp[3] + vp[2]);
        push.frustum[5] = glm::vec4(0);
        push.numDraws = m_NumDraws;


        CommandBuffer cb = m_CbManager.AquireCommandBuffer(m_LogicalDevice);
        cb.Begin();

        std::vector<VkBufferMemoryBarrier> bmbs;
        VkAccessFlags srcAccess = 0; // srcAccess is ignored for Q ownership transfers (when aquiring)
        uint32_t tf = m_GfxCaps.GetQueueFamilies().transferFamily;
        uint32_t gf = m_GfxCaps.GetQueueFamilies().graphicsFamily;

        auto& dcb = m_ShaderManager.GetDrawCommandBuffer();
        m_CbManager.AddQueueDependencies(dcb.GetTimeline());
        dcb.MarkUsedInQueue();
        // aquiring DrawCommandBuffer from Transfer Queue
        if (m_DelayTransferOperation)
        {
            m_DelayTransferOperation = false;

            const auto bmb_dcb = utils::CreateBufferMemoryBarrier(srcAccess, VK_ACCESS_SHADER_READ_BIT, gf, tf, dcb.GetBuffer());
            bmbs.push_back(bmb_dcb);

            utils::InsertBufferBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, bmbs.data(), static_cast<uint32_t>(bmbs.size()));
        }

        //auto& ddb = m_ShaderManager.GetDrawDataBuffers(m_Swapchain.GetFrameClock());
        //const auto bmb_ddb = utils::CreateBufferMemoryBarrier(srcAccess, VK_ACCESS_SHADER_READ_BIT, gf, tf, ddb.GetBuffer());
        //bmbs.push_back(bmb_ddb);
        //
        //m_CbManager.AddQueueDependencies(ddb.GetTimeline());
        //ddb.MarkUsedInQueue();

        // not sure if src stage is correct..
        // maybe try src as just before compute stage

        vkCmdBindPipeline(cb.cmb, VK_PIPELINE_BIND_POINT_COMPUTE, updateDrawsProgram.GetPipeline());
        vkCmdPushConstants(cb.cmb, updateDrawsProgram.GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
        vkCmdBindDescriptorSets(cb.cmb, VK_PIPELINE_BIND_POINT_COMPUTE, updateDrawsProgram.GetPipelineLayout(), 0, dsets.size(), dsets.data(), 0, nullptr);
        
        const auto fillMemBar = utils::CreateBufferMemoryBarrier(VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, m_ShaderManager.GetDrawCommandCountBuffer().GetBuffer());
        utils::InsertBufferBarrier(cb, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, &fillMemBar, 1);

        vkCmdFillBuffer(cb.cmb, m_ShaderManager.GetDrawCommandCountBuffer().GetBuffer(), 0, VK_WHOLE_SIZE, 0);

        const auto fillMemBar2 = utils::CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, m_ShaderManager.GetDrawCommandCountBuffer().GetBuffer());
        utils::InsertBufferBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, &fillMemBar2, 1);

        vkCmdDispatch(cb.cmb, (m_NumDraws + 31) / 32, 1, 1);

        std::array<VkBufferMemoryBarrier, 3> memBars;
        // make sure?
        memBars[0] = utils::CreateBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, m_DrawBuffer.GetBuffer());
        // make sure new draw data indices written by this CS is visible to basic.ind.vert
        memBars[1] = utils::CreateBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, m_ShaderManager.GetDrawDataIndicesBuffer().GetBuffer());
        // make sure new draw command count is visible to vkCmdDrawIndirectIndexedCount
        memBars[2] = utils::CreateBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, m_ShaderManager.GetDrawCommandCountBuffer().GetBuffer());
        
        // I wonder what happens when you have multiple pipeline stage flag bits like I do here
        utils::InsertBufferBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, memBars.data(), static_cast<uint32_t>(memBars.size()));
        
        cb.End();
        m_CbManager.SubmitInternal(cb);
    }

    // This happens before UpdateDrawCommands
    void Graphics::StartFrame()
    {
        const auto index = m_Swapchain.GetFrameClock();
        switch (m_Settings.renderMode)
        {
        case kEngineRenderModeTraditional:
            // this contains a race condition
            m_CbManager.AquireCommandBuffer(m_LogicalDevice);
            m_ShaderManager.UpdateDrawData(m_LogicalDevice, index, m_DrawData);
            break;
        case kEngineRenderModeGPUDriven:
            // should have been already waited on in engine sync, can just get it
            if (m_DelayTransferOperation)
            {
                auto& dst = m_ShaderManager.GetDrawDataBuffers(index);
                m_TransferCbManager.AddQueueDependencies(dst.GetTimeline());
                dst.MarkUsedInQueue();
            }
            break;
        }
    }

    static int first = 3;
    void Graphics::RenderCameras()
    {
        if(m_Settings.renderMode == kEngineRenderModeGPUDriven)
            Cull();

        for (const auto& camera : m_CameraData)
        {
            if (first > 0)
            {
                GlobalData data;
                data.ViewProjection = glm::translate(camera.Projection * camera.View, glm::vec3(0.0, 0.0, -100.0));
                //data.ViewProjection = camera.Projection * camera.View;
                m_ShaderManager.UpdateGlobalData(m_LogicalDevice, m_Swapchain.GetFrameClock(), data);
                m_CbManager.AddQueueDependencies(m_ShaderManager.GetGlobalDataBuffer(m_Swapchain.GetFrameClock()).GetTimeline());
                m_ShaderManager.GetGlobalDataBuffer(m_Swapchain.GetFrameClock()).MarkUsedInQueue();
                first--;
            }

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
        // Add all potential dependencies

        auto synchs = m_CbManager.SubmitToQueue(m_GfxQueue, m_LogicalDevice, kSubmitSynchForPresent, m_CurrentFrame);

        m_Swapchain.Present(m_PresentationQueue, m_CbManager.GetCommandExecSemaphores());
        m_CbManager.SignalFrameEnded();
        m_TransferCbManager.SignalFrameEnded();
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
        uint32_t bvAllocSize = 0;
        std::vector<Vertex> verts;
        std::vector<uint32_t> idxs;
        std::vector<BoundingVolumeSphere> bvs;
        const uint32_t vertBufferOffset = m_VertexBuffer.GetOffset() / sizeof(Vertex);
        const uint32_t indBufferOffset = m_IndexBuffer.GetOffset() / sizeof(uint32_t);
        for (const auto& req : meshCreationData)
        {
            // These subbuffers will be used to index and offset into the one bound Vertex and Index buffer
            VulkanSubBuffer vtxSub = VulkanSubBuffer(verts.size() + vertBufferOffset, static_cast<uint32_t>(req.vertices.size()));
            VulkanSubBuffer idxSub = VulkanSubBuffer(idxs.size() + indBufferOffset, static_cast<uint32_t>(req.indices.size()));
            verts.insert(verts.end(), req.vertices.begin(), req.vertices.end());
            idxs.insert(idxs.end(), req.indices.begin(), req.indices.end());
            bvs.push_back(req.boundingVolume);
            vtxAllocSize += static_cast<uint32_t>(req.vertices.size() * sizeof(Vertex));
            idxAllocSize += static_cast<uint32_t>(req.indices.size() * sizeof(uint32_t));
            bvAllocSize += static_cast<uint32_t>(sizeof(BoundingVolumeSphere));

            m_VertexBuffers[req.id] = { idxSub, vtxSub };
        }

        // i shouldnt have to pass these
        const auto usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const auto memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        // TODO nice-to-have: batch uploads and use transfer queueu
        // upload vertices
        UploadVulkanBuffer(usageFlags, memoryFlags, m_VertexBuffer, cb, vtxAllocSize, verts.data());

        // upload indices
        UploadVulkanBuffer(usageFlags, memoryFlags, m_IndexBuffer, cb, idxAllocSize, idxs.data());

        // upload BVs
        assert(bvAllocSize);
        UploadVulkanBuffer(usageFlags, memoryFlags, m_ShaderManager.GetBoundingVolumeBuffer(), cb, bvAllocSize, bvs.data());

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

    // until have scartch mem, i can keep this
    IGPUBuffer& Graphics::GetDrawCommandStagingBuffer()
    {
        auto& drawDataBuffer = m_StagingDrawBuffer[m_Swapchain.GetFrameClock()];
        drawDataBuffer.MakeSureNotUsedOnGPU(m_LogicalDevice);

        return drawDataBuffer;
    }

    IGPUBuffer& Graphics::GetDrawDataBuffer()
    {
        auto& drawDataBuffer = m_ShaderManager.GetDrawDataBuffers(m_Swapchain.GetFrameClock());
        //auto& drawDataBuffer = m_StagingDrawDataBuffer[m_Swapchain.GetFrameClock()];
        drawDataBuffer.MakeSureNotUsedOnGPU(m_LogicalDevice);

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
        std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily, indices.transferFamily };

        float priority = 1.0f;
        for (int queueFamilyIndex : queueFamilyIndices)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // index of the graphics family to create a queue from
            queueCreateInfo.queueCount = 1;
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

         // indexing ---
        VkPhysicalDeviceShaderDrawParametersFeatures drawParamsFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES, nullptr, VK_TRUE };
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
        VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        physical_features2.features.samplerAnisotropy = VK_TRUE;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &physical_features2);
        physical_features2.features.multiDrawIndirect = true;

        VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features12.drawIndirectCount = true;
        features12.descriptorBindingPartiallyBound = true;
        features12.runtimeDescriptorArray = true;
        features12.descriptorBindingUpdateUnusedWhilePending = true;
        features12.descriptorBindingUniformBufferUpdateAfterBind = true;
        features12.descriptorBindingStorageBufferUpdateAfterBind = true;
        features12.descriptorBindingVariableDescriptorCount = true;
        features12.timelineSemaphore = true;

        drawParamsFeature.pNext = &features12;
        physical_features2.pNext = &drawParamsFeature;
        deviceCreateInfo.pNext = &physical_features2;


        VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create a logical device!");
        printf("Vulkan Logical device successfully created!\n"); // TODO: log more stuff, like extensions used and etc.

        // TODO: Make system for multiple queues when needed
        vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily, 0, &m_GfxQueue);
        vkGetDeviceQueue(m_LogicalDevice, indices.presentationFamily, 0, &m_PresentationQueue);
        vkGetDeviceQueue(m_LogicalDevice, indices.transferFamily, 0, &m_TransferQueue);
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
        m_CbManager.Initialize(m_LogicalDevice, m_GfxCaps.GetQueueFamilies().graphicsFamily, m_Settings.swapchainImageCount);
        m_TransferCbManager.Initialize(m_LogicalDevice, m_GfxCaps.GetQueueFamilies().transferFamily, m_Settings.swapchainImageCount);
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
        static constexpr VkDeviceSize stagingDrawSize = sizeof(IndirectDrawCmd) * (kMaxDrawCount + 31);
        //static constexpr VkDeviceSize stagingDrawDataSize = sizeof(DrawDataSingle) * kMaxDrawCount;

        m_VertexBuffer          = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_IndexBuffer           = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_MeshBuffer            = m_MemoryManager.GetBuffer(m_LogicalDevice, allocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);
        m_DrawBuffer            = m_MemoryManager.GetBuffer(m_LogicalDevice, drawAllocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DeviceMemoryProps);

        for (auto i = 0; i < m_Settings.swapchainImageCount; i++)
        {
            m_StagingDrawBuffer[i] = m_MemoryManager.GetBuffer(m_LogicalDevice, stagingDrawSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_DeviceMemoryProps);
            m_StagingDrawBuffer[i].MapWholeBuffer(m_LogicalDevice);
            //m_StagingDrawDataBuffer[i] = m_MemoryManager.GetBuffer(m_LogicalDevice, stagingDrawDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_DeviceMemoryProps);
            //m_StagingDrawDataBuffer[i].MapWholeBuffer(m_LogicalDevice);
        }

        printf("[Gfx Memory] Successfully allocated %2.f MB of host domain memory and %.2f MB of device domain memory\n", (stagingDrawSize) * m_Settings.swapchainImageCount / 1024.0f / 1024.0f, (allocSize * 3 + drawAllocSize) / 1024.0f / 1024.0f);
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

    const VulkanBuffer& Graphics::GetDrawCommandCountBuffer()
    {
        return m_ShaderManager.GetDrawCommandCountBuffer();
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