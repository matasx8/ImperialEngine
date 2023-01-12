#include "CommandBuffer.h"
#include <cassert>
#include <stdio.h>

// So far we only have one type of begin info, 
// so let's not create one each time this function is called
inline constexpr VkCommandBufferBeginInfo kBeginInfo = 
{ 
	VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
	nullptr, 
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
	nullptr 
};

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
	auto res = vkBeginCommandBuffer(cmb, &kBeginInfo);
	assert(res == VK_SUCCESS);
}

void imp::CommandBuffer::End()
{
	auto res = vkEndCommandBuffer(cmb);
	assert(res == VK_SUCCESS);
}
