#include "Window.h"
#include "extern/GLFW/glfw3.h"

imp::Window::Window()
	: m_Width(0), m_Height(0), m_WindowPtr()
{
}

int imp::Window::Initialize(const std::string& name, int width, int height)
{
	if (glfwInit() == GLFW_FALSE)
		return -1;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_WindowPtr = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_WindowPtr, this);
	return 1;
}

void imp::Window::Update()
{
	glfwPollEvents();
}

bool imp::Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_WindowPtr);
}

void imp::Window::Close()
{
	glfwDestroyWindow(m_WindowPtr);
	glfwTerminate();
}

