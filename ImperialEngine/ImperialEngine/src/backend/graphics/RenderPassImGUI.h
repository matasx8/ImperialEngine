#pragma once

#pragma once
#include "backend/graphics/RenderPassBase.h"

namespace imp
{

	class RenderPassImGUI : public RenderPassBase
	{
	public:
		RenderPassImGUI();

		void Execute(Graphics& gfx, const CameraData& cam) override;
	private:


	};
}