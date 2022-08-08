#pragma once
#include <vector>
#include "backend/graphics/GraphicsCaps.h"

namespace imp
{
	namespace CmdRsc
	{
		struct MeshCreationRequest
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
		};
	}
}