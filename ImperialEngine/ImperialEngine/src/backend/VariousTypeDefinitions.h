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
		glm::vec3 pos;
		glm::vec2 tex; // tex coords (u, v)
		glm::vec3 norm;
	};

	struct alignas(16) DrawDataSingle
	{
		glm::mat4x4 Transform;
		uint32_t VertexBufferId;
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
		std::shared_ptr<std::string> fragmentSpv;
	};

	struct ComputeProgramCreationRequest
	{
		std::string shaderName;
		std::shared_ptr<std::string> spv;
	};
}