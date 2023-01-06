#pragma once

#pragma once
#include "backend/graphics/RenderPass/RenderPass.h"

namespace imp
{

	class RenderPassImGUI : public RenderPass
	{
	public:
		RenderPassImGUI();

		void Execute(Graphics& gfx, const CameraData& cam) override;
	private:


	};
}