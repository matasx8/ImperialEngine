#pragma once
#include <vector>
#include <memory>
#include <vulkan_core.h>

namespace imp
{
	class RenderPass;
	struct CameraData;
	class Swapchain;

	class RenderPassFactory
	{
	public:
		RenderPassFactory() {};
		virtual std::vector<std::shared_ptr<RenderPass>> Generate(VkDevice device, const CameraData& data, const Swapchain& swapchain) = 0;

		virtual ~RenderPassFactory() {};
	};
}