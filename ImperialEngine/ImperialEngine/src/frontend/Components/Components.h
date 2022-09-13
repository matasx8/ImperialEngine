#pragma once
#include <GLM/mat4x4.hpp>
#include <ENTT/entt.hpp>
#include "backend/VulkanBuffer.h"

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
	};

	struct Material
	{
		// !HERE
	};

	struct Mesh
	{
		entt::entity e;	// transform
	};

	struct IndexedVertexBuffers
	{
		imp::VulkanSubBuffer indices;
		imp::VulkanSubBuffer vertices;
	};
}