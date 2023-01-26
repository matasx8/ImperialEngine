#pragma once
#include <vector>
#include <ENTT/entt.hpp>
#include "backend/graphics/GraphicsCaps.h"

namespace imp
{
	namespace CmdRsc
	{
		struct MeshCreationRequest
		{
			uint32_t id;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
		};

		struct MaterialCreationRequest
		{
			std::string shaderName;
			// TODO compute-drawindirect: why are these shared ptrs?
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