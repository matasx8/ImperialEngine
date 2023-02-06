#include "CommandBufferManager.h"
#include "Utils/Pool.h"
#include "Utils/SimpleTimer.h"
#include <stdexcept>
#include <cassert>

namespace imp
{
    CommandBufferManager::CommandBufferManager(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool, SimpleTimer& timer)
        : m_BufferingMode(), m_FrameClock(), m_IsNewFrame(true), m_GfxCommandPools(), m_CommandsBuffersToSubmit(), m_SemaphoresToWaitOnSubmit(), m_SemaphoresToWaitOnSubmit2(), m_CurrentFence(), m_SemaphorePool(semaphorePool), m_FencePool(fencePool), m_Timer(timer)
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

        m_SemaphoresToWaitOnSubmit2.resize(3);

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
        // need to add fence for command buffers to know when we can reset them
        // need to add semaphore to know when we can present
        Semaphore semaphore = m_SemaphorePool.Get(device, currFrame);
        semaphore.UpdateLastUsed(currFrame);
        Semaphore semaphore2 = m_SemaphorePool.Get(device, currFrame);
        semaphore2.UpdateLastUsed(currFrame);

        // TOOD: along with semaphore we must provide where to wait.
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<VkSemaphore> semaphores;
        for (auto& sem : m_SemaphoresToWaitOnSubmit)
        {
            waitStages.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
            // Recycle used semaphores
            m_SemaphorePool.Return(sem);
            semaphores.emplace_back(sem.semaphore);
        }

        if (m_SemaphoresToWaitOnSubmit2[m_FrameClock].semaphore != VK_NULL_HANDLE)
            semaphores.emplace_back(m_SemaphoresToWaitOnSubmit2[m_FrameClock].semaphore);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
        submitInfo.pWaitSemaphores = semaphores.data(); // semaphore for swapchain image or other dependency between queue operation
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.signalSemaphoreCount = submitType == kSubmitSynchForPresent ? 2 : 1;
        VkSemaphore ssems[2] = { semaphore.semaphore, semaphore2.semaphore };
        if (submitType == kSubmitSynchForPresent)
        {
        submitInfo.pSignalSemaphores = ssems;
        }
        else
            submitInfo.pSignalSemaphores = &semaphore.semaphore;
        submitInfo.commandBufferCount = static_cast<uint32_t>(m_CommandsBuffersToSubmit.size());
        auto ccs = GetCmbs(m_CommandsBuffersToSubmit);
        submitInfo.pCommandBuffers = ccs.data();


        VkResult result = vkQueueSubmit(submitQueue, 1, &submitInfo, m_CurrentFence.fence);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to submit Command Buffer to Queue!");

        SubmitSynchPrimitives primitives = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        if (submitType == kSubmitSynchForPresent)
        {
            m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(m_CommandsBuffersToSubmit, semaphore.semaphore, m_CurrentFence.fence);
            primitives = { 0, semaphore2, VK_NULL_HANDLE };
        }
        else
        {
            m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(m_CommandsBuffersToSubmit, VK_NULL_HANDLE, m_CurrentFence.fence);
            primitives = { semaphore, 0, m_CurrentFence };
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

    void CommandBufferManager::AddQueueDependencies(const std::vector<Semaphore>& semaphores)
    {
        m_SemaphoresToWaitOnSubmit.insert(m_SemaphoresToWaitOnSubmit.end(), semaphores.begin(), semaphores.end());
    }

    void CommandBufferManager::AddQueueDependenciesForLater(Semaphore& semaphores)
    {
        m_SemaphoresToWaitOnSubmit2[m_FrameClock] = semaphores;
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
            //assert(res == VK_SUCCESS);
            timer.stop();
        }
        for (auto& buff : donePool)
            readyPool.push(buff);

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