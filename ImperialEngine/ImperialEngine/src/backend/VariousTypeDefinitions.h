#pragma once
#include "extern/GLM/vec3.hpp"
#include "extern/GLM/mat4x4.hpp"
#include "Utils/EngineStaticConfig.h"
#include <memory>
#include <string>
#include <vector>

namespace imp
{
	inline constexpr uint32_t kMaxLODCount = LOD_ENABLED ? 4 : 1;
	inline constexpr size_t kMaxMeshletVertices = MESHLET_MAX_VERTS;
	inline constexpr size_t kMaxMeshletTriangles = MESHLET_MAX_PRIMS;

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

	struct ms_MeshLOD
	{
		uint32_t meshletBufferOffset;
		uint32_t taskCount;
	};

	// TODO mesh: union
	struct alignas(16) MeshData
	{
		MeshLOD LODData[kMaxLODCount];
#if !LOD_ENABLED
		int32_t pad[2];
#endif
		BoundingVolumeSphere boundingVolume;
		int32_t     vertexOffset;
	};

	struct alignas(16) ms_MeshData
	{
		ms_MeshLOD LODData[kMaxLODCount];
#if !LOD_ENABLED
		int32_t pad[2];
#endif
		BoundingVolumeSphere boundingVolume;
		uint32_t firstTask;
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
		glm::mat4x4 Model;
		uint32_t camOutputType;
		uint32_t cameraID;
		bool dirty;
		bool preview;
		bool isRenderCamera;
	};

	// Used when VF Culling is enabled
	struct IndirectDrawCmd
	{
		uint32_t	meshDataIndex;
	};

	// Used when VF Culling is disabled and instead of VkDrawMeshTasksIndirectCommandNV
	struct ms_IndirectDrawCommand
	{
		// Read by Command Processor
		uint32_t    taskCount;
		uint32_t    firstTask;
		// Used in Mesh shader
		uint32_t    meshletBufferOffset;
		uint32_t    meshTaskCount;
	};

	struct NormalCone
	{
		glm::vec3 apex;
		int8_t cone[4];
	};

	struct alignas(16) Meshlet
	{
		uint32_t coneOffset;
		uint32_t triangleOffset;
		uint32_t vertexOffset;
		uint8_t triangleCount;
		uint8_t vertexCount;
		// 2 more bytes of extra space left
	};
	static_assert(sizeof(Meshlet) == 16);

	struct MeshCreationRequest
	{
		uint32_t id;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
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
		std::shared_ptr<std::string> taskSpv;
		std::shared_ptr<std::string> fragmentSpv;
	};

	struct ComputeProgramCreationRequest
	{
		std::string shaderName;
		std::shared_ptr<std::string> spv;
	};
}