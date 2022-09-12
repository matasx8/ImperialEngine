#pragma once
#include "backend/graphics/RenderPassGeneratorBase.h"
#include "backend/graphics/RenderPassBase.h"
#include <optional>

namespace imp
{
	class RenderPassGeneratorSimple : public RenderPassGeneratorBase
	{
	public:
		RenderPassGeneratorSimple();

		std::vector<std::shared_ptr<RenderPassBase>> GenerateRenderPasses(VkDevice device, const CameraData& data, const Swapchain& swapchain) override;
		std::shared_ptr<RenderPassBase> GenerateForImGUIPass(VkDevice device, const CameraData& data, const Swapchain& swapchain);

	private:
		std::shared_ptr<RenderPassBase> tempDefaultPass;
		std::shared_ptr<RenderPassBase> GenerateForDefaultPass(VkDevice device, const CameraData& data, const Swapchain& swapchain);
	};
}