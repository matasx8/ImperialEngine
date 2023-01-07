#pragma once
#include "backend/graphics/RenderPass/RenderPassFactory.h"
#include "Utils/NonCopyable.h"
#include <vulkan_core.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace imp
{
	struct CameraData;
	class Swapchain;
	class RenderPass;
	class RenderPassFactory;
	class VulkanGarbageCollector;

	class RenderPassGenerator : NonCopyable
	{
	public:
		RenderPassGenerator(VulkanGarbageCollector* gc);

		std::vector<std::shared_ptr<RenderPass>>& GetRenderPasses(VkDevice device, const CameraData& data, const Swapchain& swapchain);

		// Must be created beforehand because we don't have custom backend for IMGUI
		std::shared_ptr<RenderPass> GetImGUIPass(VkDevice device, const CameraData& data, const Swapchain& swapchain);

		void SetRenderPassFactory(std::unique_ptr<RenderPassFactory> factory) { m_Factory = std::move(factory); }

		void Destroy(VkDevice device);

	private:

		void DestroyRPs(std::vector<std::shared_ptr<RenderPass>>& renderPasses, VkDevice device);
		// Use Vulkan GC to delay destroy of RP
		void DestroyRPsSafely(std::vector<std::shared_ptr<RenderPass>>& renderPasses, VkDevice device);

		std::unique_ptr<RenderPassFactory> m_Factory;
		std::unordered_map<uint32_t, std::vector<std::shared_ptr<RenderPass>>> m_RenderPassMap;

		VulkanGarbageCollector* m_GC;
	};
}