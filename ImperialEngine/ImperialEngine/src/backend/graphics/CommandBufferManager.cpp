#include "CommandBufferManager.h"
#include <stdexcept>

imp::CommandBufferManager::CommandBufferManager()
{
}

void imp::CommandBufferManager::Initialize(VkDevice device, QueueFamilyIndices familyIndices, EngineSwapchainImageCount imageCount)
{
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

void imp::CommandBufferManager::SignalFrameEnded()
{
    m_FrameClock++;
    m_FrameClock %= m_BufferingMode;

    // free buffer pool somewhere next time now
}

std::vector<imp::CommandBuffer> imp::CommandBufferManager::AquireCommandBuffers(VkDevice device, uint32_t count)
{
    return m_GfxCommandPools[m_FrameClock].AquireCommandBuffers(device, count);
}

void imp::CommandBufferManager::Destroy(VkDevice device)
{
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
