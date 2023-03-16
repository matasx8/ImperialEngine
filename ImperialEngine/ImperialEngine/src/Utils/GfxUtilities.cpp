#include "GfxUtilities.h"
#include "extern/MESHOPTIMIZER/meshoptimizer.h"
#include <GLM/gtc/matrix_access.hpp>

namespace imp
{
	namespace utils
	{
		// Clip Space approach for extracting planes
		std::array<glm::vec4, 6> FindViewFrustumPlanes(const glm::mat4x4& A)
		{
			// Let's say p = (x,y,z,1) is in model space
			// Can transform it to Clip Space (in homogenous coordinates) by using an MVP matrix.
			// Performing perspective division gives us NDC

			// In Vulkan NDC the VF is an AAB centered in origin and bounded by these planes:
			// left plane x = -1
			// right plane x = 1
			// top plane y = 1
			// bottom plane y = -1
			// near plane z = 0
			// far plane z = 1

			// That means p that is in NDC is inside the VF if:
			// -1 < px < 1
			// -1 < py < 1
			//  0 < pz < 1

			// Same point p in homogenous clip space is inside VF if:
			// -pw < px < pw
			// -pw < py < pw
			// -pw < pz < pw

			// We can get px, py, pz, pw:
			// A * p

			// In our case here A is View and Projection
			// And when doing culling we'll have Model * p

			std::array<glm::vec4, 6> frustum;
			frustum[0] = utils::NormalizePlane(glm::row(A, 3) + glm::row(A, 0)); // pw + px
			frustum[1] = utils::NormalizePlane(glm::row(A, 3) - glm::row(A, 0)); // pw - px
			frustum[2] = utils::NormalizePlane(glm::row(A, 3) + glm::row(A, 1)); // pw + py
			frustum[3] = utils::NormalizePlane(glm::row(A, 3) - glm::row(A, 1)); // pw - py
			frustum[4] = utils::NormalizePlane(glm::row(A, 3) + glm::row(A, 2)); // pw + pz
			frustum[5] = utils::NormalizePlane(glm::row(A, 3) - glm::row(A, 2)); // pw - pz
			return frustum;
		}

		glm::vec4 NormalizePlane(const glm::vec4& plane)
		{
			return plane / glm::length(glm::vec3(plane));
		}

		BoundingVolumeSphere FindSphereBoundingVolume(const Vertex* vertices, size_t numVertices)
		{
			glm::vec3 pos = glm::vec3(vertices->vx, vertices->vy, vertices->vz);
			glm::vec3 minX = pos;
			glm::vec3 maxX = pos;
			glm::vec3 minY = pos;
			glm::vec3 maxY = pos;
			glm::vec3 minZ = pos;
			glm::vec3 maxZ = pos;

			for (auto i = 0; i < numVertices; i++)
			{
				if (vertices[i].vx < minX.x) minX = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
				if (vertices[i].vx > maxX.x) maxX = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
				if (vertices[i].vy < minY.y) minY = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
				if (vertices[i].vy > maxY.y) maxY = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
				if (vertices[i].vz < minZ.z) minZ = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
				if (vertices[i].vz > maxZ.z) maxZ = glm::vec3(vertices[i].vx, vertices[i].vy, vertices[i].vz);
			}

			glm::vec3 diag = (maxX - minX) + (maxY - minY) + (maxZ - minZ);
			glm::vec3 center = diag / 2.0f;
			float diameter = glm::length(diag);
			BoundingVolumeSphere bv = { center, diameter };
			return bv;
		}

		void InsertBufferBarrier(CommandBuffer& cb, VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, const VkBufferMemoryBarrier* bmbs, uint32_t bmbCount)
		{
			assert(bmbs);
			vkCmdPipelineBarrier(cb.cmb, srcFlags, dstFlags, 0, 0, 0, bmbCount, bmbs, 0, 0);
		}

		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
		{
			return CreateBufferMemoryBarrier(srcAccessMask, dstAccessMask, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, offset, size);
		}

		VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQFIndex, uint32_t dstQFIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
		{
			VkBufferMemoryBarrier bmb;
			bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bmb.pNext = nullptr;
			bmb.srcAccessMask = srcAccessMask;
			bmb.dstAccessMask = dstAccessMask;
			bmb.srcQueueFamilyIndex = srcQFIndex;
			bmb.dstQueueFamilyIndex = dstQFIndex;
			bmb.buffer = buffer;
			bmb.offset = offset;
			bmb.size = size;
			return bmb;
		}

		uint32_t ChooseMeshLODByNearPlaneDistance(const glm::mat4x4& mTransform, const BoundingVolumeSphere& bv, const glm::mat4x4& vpTransform)
		{
			uint32_t idx = 0;
			const auto hPos = vpTransform * mTransform * glm::vec4(bv.center, 1.0f);
			
			if (hPos.z - bv.diameter >= 250)
				idx = 3;
			else if (hPos.z - bv.diameter >= 100)
				idx = 2;
			else if (hPos.z - bv.diameter >= 25)
				idx = 1;
			return idx;
		}

		void GenerateMeshLODS(const std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, VulkanSubBuffer* dstSubBuffers, uint32_t numLODs, double factor, float error)
		{
			const auto FillRestOfBuffers = [&](size_t currIndexBuffOffset, uint32_t idx, uint32_t newIndexCount)
			{
				for (auto i = idx; i < numLODs; i++)
				{
					dstSubBuffers[i] = VulkanSubBuffer(currIndexBuffOffset, newIndexCount);
				}
			};

			const uint32_t* currIndices = indices.data();
			size_t currIndexCount = indices.size();
			size_t currIndexBuffOffset = 0;
			uint32_t* dst = new uint32_t[currIndexCount];

			for (uint32_t i = 0; i < numLODs; i++)
			{
				size_t indicesTarget = (size_t)((double)currIndexCount * factor);
				float err;
				size_t newIndexCount = meshopt_simplify(dst, currIndices, currIndexCount, (float*)(&vertices.front().vx), vertices.size(), sizeof(Vertex), indicesTarget, error, 0, &err);

				// Didn't change the number of indices, means won't go anymore.
				// Setting rest of LODs to last successful
				if (newIndexCount == currIndexCount)
				{
					FillRestOfBuffers(currIndexBuffOffset, i, (uint32_t)newIndexCount);
					break;
				}

				meshopt_optimizeVertexCache(dst, dst, newIndexCount, vertices.size());

				currIndexBuffOffset = indices.size();

				dstSubBuffers[i] = VulkanSubBuffer(currIndexBuffOffset, (uint32_t)newIndexCount);

				for (auto j = 0; j < newIndexCount; j++)
					indices.emplace_back(dst[j]);

				currIndices = &indices[currIndexBuffOffset];
				currIndexCount = newIndexCount;
			}

			delete[] dst;
		}

		void OptimizeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
		{
			meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

			meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));
		}

		std::vector<Meshlet> GenerateMeshlets(std::vector<Vertex>& verts, std::vector<uint32_t>& indices)
		{
			std::vector<Meshlet> meshletsDst;
			const size_t max_vertices = 64;
			const size_t max_triangles = 126;
			const size_t index_count = max_triangles * 3;
			const float cone_weight = 0.5f;

			size_t meshletBound = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
			assert(meshletBound);

			std::vector<meshopt_Meshlet> meshlets(meshletBound);
			std::vector<unsigned int> vertices(meshletBound * max_vertices);
			// this is an index buffer, but this library likes to name it as triangles
			std::vector<unsigned char> triangles(meshletBound * max_triangles * 3);

			size_t meshletCount = meshopt_buildMeshlets(meshlets.data(), vertices.data(), triangles.data(), indices.data(), indices.size(), (float*)verts.data(), verts.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);
			
			for (size_t i = 0; i < meshletCount; i++)
			{
				Meshlet meshlet = {};

				for (unsigned int j = 0; j < meshlets[i].vertex_count; j++)
					meshlet.vertices[j] = vertices[j + meshlets[i].vertex_offset];

				for (unsigned int j = 0; j < meshlets[i].triangle_count * 3; j++)
				{
					meshlet.indices[j] = static_cast<uint8_t>(triangles[j + meshlets[i].triangle_offset]);
				}


				meshlet.triangleCount = static_cast<uint8_t>(meshlets[i].triangle_count);
				meshlet.vertexCount = static_cast<uint8_t>(meshlets[i].vertex_count);

				meshopt_Bounds bounds = meshopt_computeMeshletBounds(meshlet.vertices, meshlet.indices, meshlet.triangleCount, (float*)verts.data(), verts.size(), sizeof(Vertex));
				meshlet.cone[0] = bounds.cone_axis[0];
				meshlet.cone[1] = bounds.cone_axis[1];
				meshlet.cone[2] = bounds.cone_axis[2];
				meshlet.cone[3] = bounds.cone_cutoff;
				meshletsDst.push_back(meshlet);
			}

			return meshletsDst;
		}
	}
}