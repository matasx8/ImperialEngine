#pragma once
#include "backend/graphics/RenderPassBase.h"

namespace imp
{
	class RenderPass : public RenderPassBase
	{
	public:
		RenderPass();

		void Execute() override;
	private:


	};
}