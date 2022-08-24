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
	};

	struct Camera
	{
		entt::entity e;
	};

	struct CameraComponent
	{
		float yaw;
		float pitch;
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
		imp::VulkanBuffer indices;
		imp::VulkanBuffer vertices;
	};
}