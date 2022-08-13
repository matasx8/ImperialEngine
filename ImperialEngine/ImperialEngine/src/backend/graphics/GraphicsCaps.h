#pragma once
#include <vulkan.h>
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

		// check if queue families are valid
		bool IsValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0;
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

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 tex; // tex coords (u, v)
		glm::vec3 norm;
	};

	class GraphicsCaps
	{
	public:
		GraphicsCaps();

		bool ValidationLayersSupported();
		// Check if device has required queue families and device features.
		// TODO: supply needed features, currently what's needed is hardcoded
		// TODO: cache stuff I'm querying here
		bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtens);
		bool CheckDeviceHasAnySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		const PhysicalDeviceSurfaceCaps& GetPhysicalDeviceSurfaceCaps() const;
		QueueFamilyIndices GetQueueFamilies() const;

		void SetQueueFamilies(QueueFamilyIndices& fams);

		static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		static VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes, const EngineGraphicsSettings& settings);
		static VkExtent2D ChooseBestExtent(VkSurfaceCapabilitiesKHR surfaceCaps, VkExtent2D preferredExtent);
		static uint32_t ChooseSwapchainImageCount(VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t preferred);

		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	private:

		PhysicalDeviceSurfaceCaps m_DeviceSurfaceCaps;
		QueueFamilyIndices m_QueueFamilyIndices;
	};


}