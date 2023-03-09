#pragma once
#include "extern/GLM/vec3.hpp"
#include "extern/GLM/mat4x4.hpp"
#include <memory>
#include <string>
#include <vector>

namespace imp
{
	inline constexpr uint32_t kMaxLODCount = 4;

	struct BoundingVolumeSphere
	{
		glm::vec3 center;
		float diameter;
	};

	struct MeshLOD
	{
		uint32_t indexCount;
		uint32_t firstIndex;
	};

	struct alignas(16) MeshData
	{
		MeshLOD LODData[kMaxLODCount];
		BoundingVolumeSphere boundingVolume;
		int32_t     vertexOffset;
		int32_t     pad;
	};

	struct Vertex
	{
		float vx, vy, vz;
		uint16_t nx, ny, nz, nw;
		uint16_t tu, tv;
	};

	struct alignas(16) DrawDataSingle
	{
		glm::mat4x4 Transform;
		uint32_t VertexBufferId;
		uint32_t LodIdx;
	};

	struct CameraData
	{
		glm::mat4x4 Projection;
		glm::mat4x4 View;
		uint32_t camOutputType;
		uint32_t cameraID;
		bool dirty;
		bool preview;
		bool isRenderCamera;
	};

	struct IndirectDrawCmd
	{
		uint32_t	meshDataIndex;
	};

	struct Meshlet
	{
		uint32_t vertices[64];
		uint8_t indices[126 * 3]; // up to 126 triangles
		uint8_t triangleCount;
		uint8_t vertexCount;
	};

	struct MeshCreationRequest
	{
		uint32_t id;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Meshlet> meshlets;
		BoundingVolumeSphere boundingVolume;
	};

	struct MaterialCreationRequest
	{
		std::string shaderName;
		// TODO nice-to-have: why are these shared ptrs?
		std::shared_ptr<std::string> vertexSpv;
		// Indirect variant
		std::shared_ptr<std::string> vertexIndSpv;
		// Mesh variant
		std::shared_ptr<std::string> meshSpv;
		std::shared_ptr<std::string> fragmentSpv;
	};

	struct ComputeProgramCreationRequest
	{
		std::string shaderName;
		std::shared_ptr<std::string> spv;
	};
}