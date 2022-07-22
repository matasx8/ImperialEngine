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

		void Destroy(VkInstance instance);
	private:
		int m_Width, m_Height;
		GLFWwindow* m_WindowPtr;
		VkSurfaceKHR m_Surface;
	};
}