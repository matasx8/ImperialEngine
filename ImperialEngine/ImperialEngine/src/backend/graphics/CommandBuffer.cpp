#include "CommandBuffer.h"
#include <cassert>
#include <stdio.h>

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
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	auto res = vkBeginCommandBuffer(cmb, &beginInfo);
	assert(res == VK_SUCCESS);
}

void imp::CommandBuffer::End()
{
	auto res = vkEndCommandBuffer(cmb);
	assert(res == VK_SUCCESS);
}
