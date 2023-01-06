#pragma once
#include "RenderPassFactorySimple.h";
#include "DefaultColorRP.h"
#include "DefaultDepthRP.h"
#include "backend/graphics/Graphics.h"

namespace imp
{
	std::vector<std::shared_ptr<RenderPass>> RenderPassFactorySimple::Generate(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		std::vector<std::shared_ptr<RenderPass>> RPs;

		switch (data.camOutputType)
		{
		case kCamOutColor:
		case kCamOutDepth:
			RPs.emplace_back(GenerateDefaultColorPass(device, data, swapchain));
			break;
			//RPs.emplace_back(GenerateDefaultDepthPass(device, data, swapchain));
		}

		return RPs;
	}

	std::shared_ptr<RenderPass> RenderPassFactorySimple::GenerateDefaultColorPass(VkDevice device, const CameraData & data, const Swapchain & swapchain)
	{
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
			false };

		RenderPassDesc defaultPass = { colorSurfaces, resolveSurfaces, depthDesc, colorAttachmentCount };

		auto rp = std::make_shared<DefaultColorRP>();
		rp->Create(device, defaultPass);
		return rp;
	}
	
	std::shared_ptr<RenderPass> RenderPassFactorySimple::GenerateDefaultDepthPass(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
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
			kLoadOpClear, kStoreOpStore,
			false,
			false };

		RenderPassDesc defaultPass = { colorSurfaces, resolveSurfaces, depthDesc, colorAttachmentCount };

		auto rp = std::make_shared<DefaultDepthRP>();
		rp->Create(device, defaultPass);
		return rp;
	}
}