#include "VkWindow.h"
#include "frontend/Window.h"
#include <extern/GLFW/glfw3.h>
#include <stdexcept>

imp::VkWindow::VkWindow()
	: m_Width(), m_Height(), m_WindowPtr(), m_Surface()
{
}

imp::VkWindow::VkWindow(const Window& window)
	: m_Surface()
{
	m_WindowPtr = window.GetWindowPtr();
	m_Width = window.GetWidth();
	m_Height = window.GetHeight();
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

void imp::VkWindow::Destroy(VkInstance instance)
{
	vkDestroySurfaceKHR(instance, m_Surface, nullptr);
}
