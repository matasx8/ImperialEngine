#include "CommandBufferManager.h"
#include <stdexcept>
#include <cassert>

imp::CommandBufferManager::CommandBufferManager()
    : m_BufferingMode(), m_FrameClock(), m_GfxCommandPools()
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

void imp::CommandBufferManager::Submit(VkQueue submitQueue, VkDevice device, std::vector<CommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSemaphores)
{
    // need to add fence for command buffers to know when we can reset them
    // need to add semaphore to know when we can present
    const auto semaphore = GetSemaphore(device);
    const auto fence = GetFence(device);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data(); // semaphore for swapchain image or other dependency between queue operation
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // I've no idea what to put here
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore;

    VkResult result = vkQueueSubmit(submitQueue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");

    m_GfxCommandPools[m_FrameClock].ReturnCommandBuffers(commandBuffers, semaphore, fence);
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
        m_GfxCommandPools[m_FrameClock].Reset(device);
    return m_GfxCommandPools[m_FrameClock].AquireCommandBuffers(device, count);
}

std::vector<VkSemaphore>& imp::CommandBufferManager::GetCommandExecSemaphores()
{
    return m_GfxCommandPools[m_FrameClock].semaphores;
}

void imp::CommandBufferManager::Destroy(VkDevice device)
{
    for (auto& pool : m_GfxCommandPools)
        vkDestroyCommandPool(device, pool.pool, nullptr);
}

VkSemaphore imp::CommandBufferManager::GetSemaphore(VkDevice device)
{
    VkSemaphore sem;
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    assert(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sem) == VK_SUCCESS);
    return sem;
}

VkFence imp::CommandBufferManager::GetFence(VkDevice device)
{
    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    assert(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence) == VK_SUCCESS);
    return fence;
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

    uint32_t left = count - buffers.size();
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
    semaphores.push_back(semaphore);
    fences.push_back(fence);
}

void imp::CommandPool::Reset(VkDevice device)
{
    if(fences.size())
        assert(vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max()) == VK_TRUE);
    for (auto& buff : donePool)
        readyPool.push(buff);
    // TODO: not sure if releasing resources is good?
    auto res = vkResetCommandPool(device, pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    //assert( == VK_TRUE);

    for (int i = 0; i < fences.size(); i++)
    {
        vkDestroyFence(device, fences[i], nullptr);
        vkDestroySemaphore(device, semaphores[i], nullptr);
    }
    // not sure if best and simplest way to clear vector?
    donePool = {};
    fences = {};
    semaphores = {};
}
