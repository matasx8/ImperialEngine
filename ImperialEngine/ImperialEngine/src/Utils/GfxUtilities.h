#pragma once
#include "backend/graphics/Graphics.h"
#include "backend/EngineCommandResources.h"

namespace imp
{
	namespace utils
	{
		BoundingVolumeSphere FindSphereBoundingVolume(const Vertex* vertices, size_t numVertices);
	}
}