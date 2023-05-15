#pragma once
#include "RenderPassFactory.h"

namespace imp
{
	class RenderPass;
	struct CameraData;
	class Swapchain;

	class RenderPassFactorySimple : public RenderPassFactory
	{
	public:
		std::vector<std::shared_ptr<RenderPass>> Generate(VkDevice device, const CameraData& data, const Swapchain& swapchain) override;

	private:
		std::shared_ptr<RenderPass> GenerateDefaultColorPass(VkDevice device, const CameraData& data, const Swapchain& swapchain);
	};
}