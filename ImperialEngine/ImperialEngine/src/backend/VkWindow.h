#pragma once
#include <vulkan.h>

struct GLFWwindow;

namespace imp
{
	class Window;

	class VkWindow
	{
	public:
		VkWindow();
		VkWindow(const Window& window);

		void CreateWindowSurface(VkInstance instance);
		VkSurfaceKHR GetWindowSurface() const;
		VkExtent2D GetExtent() const;

		void Destroy(VkInstance instance);
	private:
		VkExtent2D m_Extent;
		GLFWwindow* m_WindowPtr;
		VkSurfaceKHR m_Surface;
	};
}