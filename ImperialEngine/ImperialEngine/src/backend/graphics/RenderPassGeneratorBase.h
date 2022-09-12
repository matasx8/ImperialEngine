#pragma once
#include "Utils/NonCopyable.h"
#include "vulkan_core.h"
#include <vector>
#include <memory>

namespace imp
{
	struct CameraData;
	class Swapchain;
	class RenderPassBase;

	class RenderPassGeneratorBase : NonCopyable
	{
	public:
		RenderPassGeneratorBase();

		virtual std::vector<std::shared_ptr<RenderPassBase>> GenerateRenderPasses(VkDevice device, const CameraData& data, const Swapchain& swapchain) = 0;
	private:


	};
}