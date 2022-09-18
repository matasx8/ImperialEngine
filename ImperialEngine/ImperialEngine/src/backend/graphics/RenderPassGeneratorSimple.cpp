#include "RenderPassGeneratorSimple.h"
#include "backend/graphics/Graphics.h"
#include <IPROF/iprof.hpp>

namespace imp
{
	RenderPassGeneratorSimple::RenderPassGeneratorSimple()
		: tempDefaultPass(), RenderPassGeneratorBase()
	{
	}

	std::vector<std::shared_ptr<RenderPassBase>> RenderPassGeneratorSimple::GenerateRenderPasses(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		IPROF_FUNC;
		std::vector<std::shared_ptr<RenderPassBase>> renderPasses;
		// read camera data ant generate pod that can be used to either fetch existing RP or generate new ones
		renderPasses.emplace_back(GenerateForDefaultPass(device, data, swapchain));

		// auto imGuiRP = GenerateForImGUIPass(); cant do while workaround is used for imgui
		return renderPasses;
	}

	std::shared_ptr<RenderPassBase> RenderPassGeneratorSimple::GenerateForDefaultPass(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		if (tempDefaultPass)	// temporary until we need more than 1 pass or something more dynamic
			return tempDefaultPass;

		const uint8_t colorAttachmentCount = 1;
		std::array<SurfaceDesc, kMaxColorAttachmentCount> colorSurfaces = {};
		colorSurfaces[0] = swapchain.GetSwapchainImageSurfaceDesc();
		colorSurfaces[0].loadOp = kLoadOpClear; // get from camera data
		colorSurfaces[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
			false};

		RenderPassDesc defaultPass = { colorSurfaces, resolveSurfaces, depthDesc, colorAttachmentCount};

		auto rp = std::make_shared<RenderPass>();
		rp->Create(device, defaultPass);
		tempDefaultPass = rp;
		return rp;
	}

	std::shared_ptr<RenderPassBase> RenderPassGeneratorSimple::GenerateForImGUIPass(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		const uint8_t colorAttachmentCount = 1;
		std::array<SurfaceDesc, kMaxColorAttachmentCount> colorSurfaces = {};
		colorSurfaces[0] = swapchain.GetSwapchainImageSurfaceDesc();
		colorSurfaces[0].loadOp = kLoadOpLoad;
		colorSurfaces[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		std::array<SurfaceDesc, kMaxColorAttachmentCount> resolveSurfaces = {};
		const SurfaceDesc depthDesc = {};

		RenderPassDesc imguiPass = { colorSurfaces, resolveSurfaces, depthDesc, colorAttachmentCount };

		auto rp = std::make_shared<RenderPassImGUI>();
		rp->Create(device, imguiPass);
		return rp;
	}
}