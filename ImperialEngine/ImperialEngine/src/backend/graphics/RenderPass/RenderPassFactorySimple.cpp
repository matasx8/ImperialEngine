#pragma once
#include "RenderPassFactorySimple.h"
#include "DefaultColorRP.h"
#include "backend/graphics/Graphics.h"
#include "Utils/EngineStaticConfig.h"

namespace imp
{
	std::vector<std::shared_ptr<RenderPass>> RenderPassFactorySimple::Generate(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		std::vector<std::shared_ptr<RenderPass>> RPs;

		switch (data.camOutputType)
		{
		case kCamOutColor:
			RPs.emplace_back(GenerateDefaultColorPass(device, data, swapchain));
			break;
		}

		return RPs;
	}

	std::shared_ptr<RenderPass> RenderPassFactorySimple::GenerateDefaultColorPass(VkDevice device, const CameraData & data, const Swapchain & swapchain)
	{
		const uint8_t colorAttachmentCount = 1;
		std::array<SurfaceDesc, kMaxColorAttachmentCount> colorSurfaces = {};
		colorSurfaces[0] = swapchain.GetSwapchainImageSurfaceDesc();
		colorSurfaces[0].loadOp = kLoadOpClear; // get from camera data
#if BENCHMARK_MODE
		colorSurfaces[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#else
		colorSurfaces[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif
		std::array<SurfaceDesc, kMaxColorAttachmentCount> resolveSurfaces = {};
		const SurfaceDesc depthDesc = {
			colorSurfaces[0].width,
			colorSurfaces[0].height,
			VK_FORMAT_D32_SFLOAT,
			1,
			0, // udnefined
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			kLoadOpClear, kStoreOpDontCare,
			false,
			false };

		RenderPassDesc defaultPass = { colorSurfaces, resolveSurfaces, depthDesc, colorAttachmentCount };

		auto rp = std::make_shared<DefaultColorRP>();
		rp->Create(device, defaultPass);
		return rp;
	}
}