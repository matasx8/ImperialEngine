#pragma once
#include <vulkan.h>
#include <vector>

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
		QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	private:

	};


}