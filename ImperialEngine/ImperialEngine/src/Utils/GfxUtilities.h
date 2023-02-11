#pragma once
#include "backend/graphics/Graphics.h"
#include "backend/EngineCommandResources.h"

namespace imp
{
	namespace utils
	{
		BoundingVolumeSphere FindSphereBoundingVolume(const Vertex* vertices, size_t numVertices);
		void InsertBufferBarrier(CommandBuffer& cb, VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, const VkBufferMemoryBarrier* bmbs, uint32_t bmbCount);
		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQFIndex, uint32_t dstQFIndex, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	}
}