#include "CommandBuffer.h"

imp::CommandBuffer::CommandBuffer()
	: cmb()
{
}

imp::CommandBuffer::CommandBuffer(VkCommandBuffer cb)
	: cmb(cb)
{
}
