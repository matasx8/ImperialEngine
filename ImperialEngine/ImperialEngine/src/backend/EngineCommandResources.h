#pragma once
#include "extern/GLM/vec3.hpp"
#include "backend/graphics/GraphicsCaps.h"
#include <vector>
#include <ENTT/entt.hpp>

namespace imp
{
	// TODO acceleration-part-1: put this somewhere else
	struct SphereBV
	{
		glm::vec3 center;
		float radius;
	};

	namespace CmdRsc
	{

		struct MeshCreationRequest
		{
			uint32_t id;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			SphereBV boundingVolume;
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
}