#include "RenderPassGeneratorBase.h"
#include "RenderPass/RenderPassFactory.h"
#include "backend/VulkanGarbageCollector.h"
#include "Graphics.h"
#include <cassert>

namespace imp
{
	RenderPassGenerator::RenderPassGenerator(VulkanGarbageCollector* gc)
		: m_Factory(), m_RenderPassMap(), m_GC(gc)
	{
	}

	std::vector<std::shared_ptr<RenderPass>>& RenderPassGenerator::GetRenderPasses(VkDevice device, const CameraData& data, const Swapchain& swapchain)
	{
		assert(m_Factory.get());

		auto RenderPassCache = m_RenderPassMap.find(data.cameraID);
		if (data.dirty || RenderPassCache == m_RenderPassMap.end())
		{
			// Probably won't be having the need for fancy RP cache any time soon so delete old ones
			if (RenderPassCache != m_RenderPassMap.end())
				DestroyRPsSafely(RenderPassCache->second, device);

			m_RenderPassMap[data.cameraID] = m_Factory->Generate(device, data, swapchain);
		}

		return m_RenderPassMap.at(data.cameraID);
	}

	std::shared_ptr<RenderPass> RenderPassGenerator::GetImGUIPass(VkDevice device, const CameraData& data, const Swapchain& swapchain)
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

	void RenderPassGenerator::Destroy(VkDevice device)
	{
		for (auto& cache : m_RenderPassMap)
			DestroyRPs(cache.second, device);
	}

	void RenderPassGenerator::DestroyRPs(std::vector<std::shared_ptr<RenderPass>>& renderPasses, VkDevice device)
	{
		for (auto& rp : renderPasses)
			rp->Destroy(device);
	}

	void RenderPassGenerator::DestroyRPsSafely(std::vector<std::shared_ptr<RenderPass>>& renderPasses, VkDevice device)
	{
		for (auto& rp : renderPasses)
			m_GC->AddGarbageResource(rp);
	}
}