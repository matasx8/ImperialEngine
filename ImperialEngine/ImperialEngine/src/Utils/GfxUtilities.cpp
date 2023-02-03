#include "GfxUtilities.h"

namespace imp
{
	namespace utils
	{
		// Ritter's efficient algorithm for near-optimal sphere BV creation
		SphereBV FindSphereBoundingVolume(const Vertex* vertices, size_t numVertices)
		{
			glm::vec3 minX = vertices->pos;
			glm::vec3 maxX = vertices->pos;
			glm::vec3 minY = vertices->pos;
			glm::vec3 maxY = vertices->pos;
			glm::vec3 minZ = vertices->pos;
			glm::vec3 maxZ = vertices->pos;

			for (auto i = 0; i < numVertices; i++)
			{
				const auto& vert = vertices[i].pos;
				if (vert.x < minX.x) minX = vert;
				if (vert.x > maxX.x) maxX = vert;
				if (vert.y < minY.y) minY = vert;
				if (vert.y > maxY.y) maxY = vert;
				if (vert.z < minZ.z) minZ = vert;
				if (vert.z > maxZ.z) maxZ = vert;
			}

			glm::vec3 diag = (maxX - minX) + (maxY - minY) + (maxZ - minZ);
			glm::vec3 center = diag / 2.0f;
			float radius = glm::length(center);
			SphereBV bv = { center, radius };
			return bv;
		}
			//// 1. Find the vertex that is at the minimum and the vertex at the maximum along each of the x, y and z axes.
			//std::array<glm::vec3, 6> verts = { (*vertices).pos , (*vertices).pos , (*vertices).pos, (*vertices).pos, (*vertices).pos, (*vertices).pos };
			//for (auto i = 0; i < numVertices; i++)
			//{
			//	const auto& vert = vertices[i].pos;
			//	for (auto j = 0; j < 3; j ++)
			//	{
			//		if((&vert)[j] < verts[j])
			//	}
			//	if (vert.x < minX.x) minX = vert;
			//	if (vert.x > maxX.x) maxX = vert;
			//	if (vert.y < minY.y) minY = vert;
			//	if (vert.y > maxY.y) maxY = vert;
			//	if (vert.z < minZ.z) minZ = vert;
			//	if (vert.z > maxZ.z) maxZ = vert;
			//}

			//// 2. For these pairs of vertices, find the pair with the largest distance between them
			//// 3. Use this pair to form a sphere with its center at the midpoint and a radius equal to the distance to them
			//float maxDist = glm::distance(maxX, minX);
			//glm::vec3 center = (maxX + minX) * 0.5f;
			//if()
	}
}