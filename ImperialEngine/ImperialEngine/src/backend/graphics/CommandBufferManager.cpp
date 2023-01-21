#include "CommandBufferManager.h"
#include "Utils/Pool.h"
#include <stdexcept>
#include <cassert>
#include <IPROF/iprof.hpp>

imp::CommandBufferManager::CommandBufferManager(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool)
    : m_BufferingMode(), m_FrameClock(), m_IsNewFrame(true), m_GfxCommandPools(), m_SemaphorePool(semaphorePool), m_FencePool(fencePool)
{
}

void imp::CommandBufferManager::Initialize(VkDevice device, QueueFamilyIndices familyIndices, EngineSwapchainImageCount imageCount)
{
    m_BufferingMode = static_cast<uint32_t>(imageCount);
	for (int i = 0; i < imageCount; i++)
	{
        VkCommandPoolCreateInfo poolInfo;
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = 0; // we're resetting whole pools so no need for reset flag bit.
        poolInfo.queueFamilyIndex = familyIndices.graphicsFamily;
        poolInfo.pNext = nullptr;

        VkCommandPool pool;
        VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &pool);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create a command pool");

        m_GfxCommandPools.emplace_back(pool);
	}
}

static std::vector<VkCommandBuffer> GetCmbs(std::vector<imp::CommandBuffer> commandBuffers)
{
    std::vector<VkCommandBuffer> buffs;
    for (auto& buff : commandBuffers)
        buffs.push_back(buff.cmb);
    return buffs;
}

imp::SubmitSynchPrimitives imp::CommandBufferManager::Submit(VkQueue submitQueue, VkDevice device, std::vector<CommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSemaphores, SubmitType submitType, uint64_t currFrame)
{
    IPROF_FUNC;
    // need to add fence for command buffers to know when we can reset them
    // need to add semaphore to know when we can present
    Semaphore semaphore = m_SemaphorePool.Get(device, currFrame);
    semaphore.UpdateLastUsed(currFrame);
    Fence fence = m_FencePool.Get(device, currFrame);
    fence.UpdateLastUsed(currFrame);

    // TOOD: along with semaphore we must provide where to wait. For quick hac I'm assuming this because
    // currently all additional semaphores are from transfer operations
    std::vector<VkPipelineStageFlags> waitStages;
    for (int i = 0; i < waitSemaphores.size(); i++)
    {
        if (i == 0)
            waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        else
            waitStages.push_back(VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Recycle used semaphores
        m_SemaphorePool.Return(Semaphore(waitSemaphores[i]));
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data(); // semaphore for swapchain image or other dependency between queue operation
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore.semaphore;
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    auto ccs = GetCmbs(commandBuffers);
    submitInfo.pCommandBuffers = ccs.data();


    VkResult result = vkQueueSubmit(submitQueue, 1, &submitInfo, fence.fence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");

    if (submitType == kSubmitSynchForPresent)
    {
        m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(commandBuffers, semaphore.semaphore, fence.fence);
        return { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }
    else
    {
        m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(commandBuffers, VK_NULL_HANDLE, fence.fence);
        return { semaphore, fence };
    }
}

void imp::CommandBufferManager::SignalFrameEnded()
{
    m_FrameClock++;
    m_FrameClock %= m_BufferingMode;
    m_IsNewFrame = true;
}

std::vector<imp::CommandBuffer> imp::CommandBufferManager::AquireCommandBuffers(VkDevice device, uint32_t count)
{
    if (m_IsNewFrame)
    {
        m_GfxCommandPools[m_FrameClock].Reset(device, m_SemaphorePool, m_FencePool);
        m_IsNewFrame = false;
    }
    return m_GfxCommandPools[m_FrameClock].AquireCommandBuffers(device, count);
}

imp::CommandBuffer imp::CommandBufferManager::AquireCommandBuffer(VkDevice device)
{
    return AquireCommandBuffers(device, 1)[0];
}

std::vector<VkSemaphore>& imp::CommandBufferManager::GetCommandExecSemaphores()
{
    return m_GfxCommandPools[m_FrameClock].semaphores;
}

void imp::CommandBufferManager::Destroy(VkDevice device)
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

std::vector<imp::CommandBuffer> imp::CommandPool::AquireCommandBuffers(VkDevice device, uint32_t count)
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

void imp::CommandPool::ReturnCommandBuffers(std::vector<CommandBuffer>& buffers, VkSemaphore semaphore, VkFence fence)
{
    donePool.insert(donePool.end(), buffers.begin(), buffers.end());
    if(semaphore != VK_NULL_HANDLE)
        semaphores.push_back(semaphore);
    if(fence != VK_NULL_HANDLE)
        fences.push_back(fence);
}

void imp::CommandPool::Reset(VkDevice device, PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool)
{
    if (fences.size())
    {
        const auto res = vkWaitForFences(device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, ~0ull);
        assert(res == VK_SUCCESS);
    }
    for (auto& buff : donePool)
        readyPool.push(buff);

    auto res = vkResetCommandPool(device, pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    assert(res == VK_SUCCESS);

    if(fences.size())
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
