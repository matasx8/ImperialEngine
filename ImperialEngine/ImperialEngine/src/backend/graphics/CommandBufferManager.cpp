#include "CommandBufferManager.h"
#include "Utils/Pool.h"
#include "Utils/SimpleTimer.h"
#include <stdexcept>
#include <cassert>

namespace imp
{
    CommandBufferManager::CommandBufferManager(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool, SimpleTimer& timer)
        : m_BufferingMode(), m_FrameClock(), m_IsNewFrame(true), m_GfxCommandPools(), m_CommandsBuffersToSubmit(), m_SemaphoresToWaitOnSubmit(), m_CurrentFence(), m_TransferCB(), m_QueueDependencies(), m_SemaphorePool(semaphorePool), m_FencePool(fencePool), m_Timer(timer)
    {
    }

    void CommandBufferManager::Initialize(VkDevice device, uint32_t familyIndices, EngineSwapchainImageCount imageCount)
    {
        m_BufferingMode = static_cast<uint32_t>(imageCount);
        for (int i = 0; i < imageCount; i++)
        {
            VkCommandPoolCreateInfo poolInfo;
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = 0; // we're resetting whole pools so no need for reset flag bit.
            poolInfo.queueFamilyIndex = familyIndices;
            poolInfo.pNext = nullptr;

            VkCommandPool pool;
            VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &pool);
            if (result != VK_SUCCESS)
                throw std::runtime_error("Failed to create a command pool");

            m_GfxCommandPools.emplace_back(pool);
        }

        // initial acquire
        m_TransferCB.InitializeEmpty();

        m_CurrentFence = m_FencePool.Get(device, 0ull);
    }

    static std::vector<VkCommandBuffer> GetCmbs(std::vector<CommandBuffer> commandBuffers)
    {
        std::vector<VkCommandBuffer> buffs;
        for (auto& buff : commandBuffers)
            buffs.push_back(buff.cmb);
        return buffs;
    }

    void CommandBufferManager::SubmitInternal(CommandBuffer& cb)
    {
        m_CommandsBuffersToSubmit.emplace_back(cb);
    }    
    
    void CommandBufferManager::SubmitInternal(CommandBuffer& cb, const std::vector<Semaphore>& semaphores)
    {
        SubmitInternal(cb);
        m_SemaphoresToWaitOnSubmit.insert(m_SemaphoresToWaitOnSubmit.end(), semaphores.begin(), semaphores.end());
    }

    SubmitSynchPrimitives CommandBufferManager::SubmitToQueue(VkQueue submitQueue, VkDevice device, SubmitType submitType, uint64_t currFrame)
    {
        // maxTimelineSemaphoreValueDifference 
        // need to add fence for command buffers to know when we can reset them
        // need to add semaphore to know when we can present
        Semaphore semaphore = m_SemaphorePool.Get(device, currFrame);
        semaphore.UpdateLastUsed(currFrame);

        std::vector<VkSemaphore> waitSemaphores(m_QueueDependencies.size());
        std::vector<VkPipelineStageFlags> waitStages(m_QueueDependencies.size());
        std::vector<uint64_t> waitValues(m_QueueDependencies.size());
        std::vector<VkSemaphore> signalSemaphores(m_QueueDependencies.size());
        std::vector<uint64_t> signalValues(m_QueueDependencies.size());

        VkTimelineSemaphoreSubmitInfo tssi = {};
        tssi.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        for (auto i = 0; const auto & sem : m_QueueDependencies)
        {
            waitValues[i] = sem.lastUsedInQueue;
            signalValues[i] = sem.lastUsedInQueue + 1ull;
            waitSemaphores[i] = sem.semaphore;
            signalSemaphores[i] = sem.semaphore;
            waitStages[i] = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            i++;
        }

        // binary semaphores
        for (auto& sem : m_SemaphoresToWaitOnSubmit)
        {
            waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            // Recycle used semaphores
            m_SemaphorePool.Return(sem);
            waitSemaphores.emplace_back(sem.semaphore);
            waitValues.push_back(1);
        }

        signalSemaphores.push_back(semaphore.semaphore);
        signalValues.push_back(1);

        tssi.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        tssi.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        tssi.pWaitSemaphoreValues = waitValues.data();
        tssi.pSignalSemaphoreValues = signalValues.data();

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = m_QueueDependencies.size() ? &tssi : nullptr;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();
        submitInfo.commandBufferCount = static_cast<uint32_t>(m_CommandsBuffersToSubmit.size());
        auto ccs = GetCmbs(m_CommandsBuffersToSubmit);
        submitInfo.pCommandBuffers = ccs.data();


        VkResult result = vkQueueSubmit(submitQueue, 1, &submitInfo, m_CurrentFence.fence);
        if (result != VK_SUCCESS)
        {
            printf("Failed result was: %i\n", result);
        }

        SubmitSynchPrimitives primitives = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        if (submitType == kSubmitSynchForPresent)
        {
            m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(m_CommandsBuffersToSubmit, semaphore.semaphore, m_CurrentFence.fence);
        }
        else
        {
            m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(m_CommandsBuffersToSubmit, VK_NULL_HANDLE, m_CurrentFence.fence);
            primitives = { semaphore, m_CurrentFence };
        }

        m_CurrentFence = m_FencePool.Get(device, currFrame);
        m_CurrentFence.UpdateLastUsed(currFrame);
        m_CommandsBuffersToSubmit.resize(0ull);
        m_SemaphoresToWaitOnSubmit.resize(0ull);

        return primitives;
    }

    void CommandBufferManager::SignalFrameEnded()
    {
        m_FrameClock++;
        m_FrameClock %= m_BufferingMode;
        m_IsNewFrame = true;

        m_QueueDependencies.resize(0);
    }

    std::vector<CommandBuffer> CommandBufferManager::AquireCommandBuffers(VkDevice device, uint32_t count)
    {
        if (m_IsNewFrame)
        {
            m_GfxCommandPools[m_FrameClock].Reset(device, m_SemaphorePool, m_FencePool, m_Timer);
            m_IsNewFrame = false;
        }
        return m_GfxCommandPools[m_FrameClock].AquireCommandBuffers(device, count);
    }

    CommandBuffer CommandBufferManager::AquireCommandBuffer(VkDevice device)
    {
        return AquireCommandBuffers(device, 1)[0];
    }

    std::vector<VkSemaphore>& CommandBufferManager::GetCommandExecSemaphores()
    {
        return m_GfxCommandPools[m_FrameClock].semaphores;
    }

    const Fence& CommandBufferManager::GetCurrentFence() const
    {
        return m_CurrentFence;
    }

    void imp::CommandBufferManager::ReturnCommandBufferToPool(CommandBuffer cb, VkSemaphore sem, VkFence fence)
    {
        std::vector<CommandBuffer> cbs = { cb };
        m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(cbs, sem, fence);
    }

    CommandBuffer& CommandBufferManager::GetCurrentCB(VkDevice device)
    {
        if (m_TransferCB.GetCurrentStage() == kCBStageDone)
        {
            m_TransferCB = AquireCommandBuffer(device);
            m_TransferCB.Begin();
        }
        else if (m_TransferCB.GetCurrentStage() == kCBStageNew)
            m_TransferCB.Begin();
        return m_TransferCB;
    }

    void CommandBufferManager::SubmitToTransferQueue(VkQueue transferQueue, VkDevice device, uint64_t currFrame)
    {
        std::vector<VkSemaphore> waitSemaphores(m_QueueDependencies.size());
        std::vector<VkPipelineStageFlags> waitStages(m_QueueDependencies.size());
        std::vector<uint64_t> waitValues(m_QueueDependencies.size());
        std::vector<uint64_t> signalValues(m_QueueDependencies.size());

        VkTimelineSemaphoreSubmitInfo tssi = {};
        tssi.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        tssi.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        tssi.pWaitSemaphoreValues = waitValues.data();
        tssi.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        tssi.pSignalSemaphoreValues = signalValues.data();

        for (auto i = 0; const auto& sem : m_QueueDependencies)
        {
            waitValues[i] = sem.lastUsedInQueue;
            signalValues[i] = sem.lastUsedInQueue + 1ull;
            waitSemaphores[i] = sem.semaphore;
            waitStages[i] = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            i++;
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &tssi;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pSignalSemaphores = waitSemaphores.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_TransferCB.cmb;

        VkResult result = vkQueueSubmit(transferQueue, 1, &submitInfo, m_CurrentFence.fence);
        if (result != VK_SUCCESS)
        {
            printf("Failed result was: %i\n", result);
        }

        m_CurrentFence = m_FencePool.Get(device, currFrame);
        m_CurrentFence.UpdateLastUsed(currFrame);
    }

    void CommandBufferManager::AddQueueDependencies(const TimelineSemaphore& semahpore)
    {
        m_QueueDependencies.push_back(semahpore);
    }

    void CommandBufferManager::Destroy(VkDevice device)
    {
        for (auto& pool : m_GfxCommandPools)
        {
            vkDestroyCommandPool(device, pool.pool, nullptr);
            for (auto& fence : pool.fences)
                vkDestroyFence(device, fence, nullptr);
            for (auto& sem : pool.semaphores)
                vkDestroySemaphore(device, sem, nullptr);
        }
    }

    std::vector<CommandBuffer> CommandPool::AquireCommandBuffers(VkDevice device, uint32_t count)
    {
        std::vector<CommandBuffer> buffers;
        for (uint32_t i = 0; i < count; i++)
        {
            if (readyPool.size() == 0)
                break;
            buffers.emplace_back(readyPool.front());
            readyPool.pop();
        }

        uint32_t left = count - static_cast<uint32_t>(buffers.size());
        if (left)
        {
            VkCommandBufferAllocateInfo cbAllocInfo = {};
            cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cbAllocInfo.commandPool = pool;
            cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cbAllocInfo.commandBufferCount = left;
            std::vector<VkCommandBuffer> newBufs(left);
            VkResult result = vkAllocateCommandBuffers(device, &cbAllocInfo, newBufs.data());
            if (result != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate Command Buffers");

            for (auto& buff : newBufs)
                buffers.emplace_back(buff);
        }
        return buffers;
    }

    void CommandPool::ReturnCommandBuffers(std::vector<CommandBuffer>& buffers, VkSemaphore semaphore, VkFence fence)
    {
        donePool.insert(donePool.end(), buffers.begin(), buffers.end());
        if (semaphore != VK_NULL_HANDLE)
            semaphores.push_back(semaphore);
        if (fence != VK_NULL_HANDLE)
            fences.push_back(fence);
    }

    void CommandPool::Reset(VkDevice device, PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool, SimpleTimer& timer)
    {
        if (fences.size())
        {
            timer.start();
            const auto res = vkWaitForFences(device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, 9999999);
            assert(res == VK_SUCCESS);
            timer.stop();
        }
        for (auto& buff : donePool)
        {
            buff.ResetStageToNew();
            readyPool.push(buff);
        }

        auto res = vkResetCommandPool(device, pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        assert(res == VK_SUCCESS);

        if (fences.size())
            vkResetFences(device, fences.size(), fences.data());

        for (int i = 0; i < fences.size(); i++)
        {
            fencePool.Return(fences[i]);
        }

        for (int i = 0; i < semaphores.size(); i++)
            semaphorePool.Return(semaphores[i]);

        donePool.resize(0);
        fences.resize(0);
        semaphores.resize(0);
    }
}