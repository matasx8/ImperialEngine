#pragma once
#include "RenderPass.h"

namespace imp
{
	class DefaultDepthRP : public RenderPass
	{
	public:
		DefaultDepthRP();

		void Execute(Graphics& gfx, const CameraData& cam) override;
	private:


	};
}
