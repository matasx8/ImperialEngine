#include "CommandBuffer.h"
#include <cassert>

imp::CommandBuffer::CommandBuffer()
	: cmb()
{
}

imp::CommandBuffer::CommandBuffer(VkCommandBuffer cb)
	: cmb(cb)
{
}

void imp::CommandBuffer::Begin()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	assert(vkBeginCommandBuffer(cmb, &beginInfo) == VK_SUCCESS);
}

void imp::CommandBuffer::End()
{
	assert(vkEndCommandBuffer(cmb) == VK_SUCCESS);
}
