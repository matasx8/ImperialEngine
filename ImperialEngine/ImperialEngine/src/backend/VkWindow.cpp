#include "VkWindow.h"
#include "frontend/Window.h"
#include <extern/GLFW/glfw3.h>
#include <stdexcept>

imp::VkWindow::VkWindow()
	: m_Extent(), m_WindowPtr(), m_Surface()
{
}

imp::VkWindow::VkWindow(const Window& window)
	: m_Surface()
{
	m_WindowPtr = window.GetWindowPtr();
	m_Extent.height = window.GetHeight();
	m_Extent.width = window.GetWidth();
}

void imp::VkWindow::CreateWindowSurface(VkInstance instance)
{
	VkResult result = glfwCreateWindowSurface(instance, m_WindowPtr, nullptr, &m_Surface);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create a surface!");
}

VkSurfaceKHR imp::VkWindow::GetWindowSurface() const
{
	return m_Surface;
}

VkExtent2D imp::VkWindow::GetExtent() const
{
	return m_Extent;
}

void imp::VkWindow::Destroy(VkInstance instance)
{
	vkDestroySurfaceKHR(instance, m_Surface, nullptr);
}
