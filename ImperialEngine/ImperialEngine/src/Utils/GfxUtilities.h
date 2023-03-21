#pragma once
#include "backend/graphics/Graphics.h"

namespace imp
{

	namespace utils
	{
		std::array<glm::vec4, 6> FindViewFrustumPlanes(const glm::mat4x4& A);
		glm::vec4 NormalizePlane(const glm::vec4& plane);
		BoundingVolumeSphere FindSphereBoundingVolume(const Vertex* vertices, size_t numVertices);
		void InsertBufferBarrier(CommandBuffer& cb, VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, const VkBufferMemoryBarrier* bmbs, uint32_t bmbCount);
		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQFIndex, uint32_t dstQFIndex, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	
		uint32_t ChooseMeshLODByNearPlaneDistance(const glm::mat4x4& mTransform, const BoundingVolumeSphere& bv, const glm::mat4x4& vpTransform);
		void GenerateMeshLODS(const std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, VulkanSubBuffer* dstSubBuffers, uint32_t numLODs, double factor, float error);
		void OptimizeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
		std::vector<Meshlet> GenerateMeshlets(std::vector<Vertex>& verts, std::vector<uint32_t>& indices, const Comp::MeshGeometry& geometry, ms_MeshData& meshData);


	}
}