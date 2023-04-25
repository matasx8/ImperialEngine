#pragma once
#include "backend/graphics/RenderPass/RenderPass.h"

namespace imp
{

	class DefaultColorRP : public RenderPass
	{
	public:
		DefaultColorRP();

		void Execute(Graphics& gfx, const CameraData& cam) override;
	};
}