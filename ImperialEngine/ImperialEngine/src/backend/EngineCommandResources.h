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
			std::shared_ptr<std::string> vertexSpv;
			std::shared_ptr<std::string> fragmentSpv;
		};
	}
}