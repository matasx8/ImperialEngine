#pragma once
#include "volk.h"
#include <vector>
#include <extern/GLM/vec2.hpp>
#include <extern/GLM/vec3.hpp>
#include "frontend/EngineSettings.h"

namespace imp
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentationFamily = -1;
		int transferFamily = -1;

		// check if queue families are valid
		bool IsValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0 && transferFamily >= 0;
		}
	};

	struct PhysicalDeviceSurfaceCaps
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentationModes;
	};

	struct MemoryProps
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;

		uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags flags) const;
	};

	class GraphicsCaps
	{
	public:
		GraphicsCaps();

		bool ValidationLayersSupported();
		bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
		std::vector<const char*> CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requestedExtens) const;
		bool CheckDeviceHasAnySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		const PhysicalDeviceSurfaceCaps& GetPhysicalDeviceSurfaceCaps() const;
		QueueFamilyIndices GetQueueFamilies() const;

		void CollectSupportedFeatures(VkPhysicalDevice device, const std::vector<const char*>& extensionsUsed);

		void SetQueueFamilies(QueueFamilyIndices& fams);

		bool IsMeshShadingSupported() const;

		static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		static VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes, const EngineGraphicsSettings& settings);
		static VkExtent2D ChooseBestExtent(VkSurfaceCapabilitiesKHR surfaceCaps, VkExtent2D preferredExtent);
		static uint32_t ChooseSwapchainImageCount(VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t preferred);

		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	private:

		PhysicalDeviceSurfaceCaps m_DeviceSurfaceCaps;
		QueueFamilyIndices m_QueueFamilyIndices;
		bool m_MeshShadingSupported;
	};


}