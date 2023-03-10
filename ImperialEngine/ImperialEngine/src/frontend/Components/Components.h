#pragma once
#include <GLM/mat4x4.hpp>
#include <ENTT/entt.hpp>
#include "backend/VulkanBuffer.h"
#include "backend/VariousTypeDefinitions.h"

enum CameraOutput : uint32_t
{
	kCamOutColor = 1U,
	kCamOutDepth
};
// seperate components and entities

namespace Comp
{
	struct Transform
	{
		glm::mat4x4 transform;

		glm::vec3 GetPosition() const;
		glm::vec4& GetPosition();
	};


	struct Camera
	{
		glm::mat4x4 projection;
		glm::mat4x4 view;
		uint32_t camOutputType;
		bool dirty;
		bool preview;
		bool isRenderCamera;
	};

	struct Material
	{
		uint32_t materialId;
	};

	struct Mesh
	{
		uint32_t meshId;
	};

	// child component points to main entity - allows to have multiple components
	struct ChildComponent
	{
		entt::entity parent; 
	};

	struct MeshGeometry
	{
		imp::VulkanSubBuffer vertices;
		imp::VulkanSubBuffer indices[imp::kMaxLODCount];
		uint32_t meshletCount;
		uint32_t meshletOffset; // offset into meshlet buffer
	};
}