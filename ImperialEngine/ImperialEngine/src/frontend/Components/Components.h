#pragma once
#include <GLM/mat4x4.hpp>
#include <ENTT/entt.hpp>
#include "backend/VulkanBuffer.h"

namespace Comp
{
	struct Transform
	{
		glm::mat4x4 transform;
	};

	struct Material
	{

	};

	struct Mesh
	{
		entt::entity e;	// transform
	};

	struct IndexedVertexBuffers
	{
		imp::VulkanBuffer indices;
		imp::VulkanBuffer vertices;
	};
}