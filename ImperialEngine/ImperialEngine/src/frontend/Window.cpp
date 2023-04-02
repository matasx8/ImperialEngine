#include "frontend/Window.h"
#include "extern/GLFW/glfw3.h"
#include "Debug.h"
#include "extern/IMGUI/backends/imgui_impl_glfw.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "extern/STB/stb_image.h"
#include "Utils/EngineStaticConfig.h"
#include <assert.h>

imp::Window::Window()
	: m_Width(0), m_Height(0), m_WindowPtr()
{
}

static void LoadLogo(GLFWwindow* window)
{
	GLFWimage image;
	image.pixels = stbi_load("DefaultResources/ImperialEngineLogo.png", &image.width, &image.height, 0, 4);
	glfwSetWindowIcon(window, 1, &image);
	stbi_image_free(image.pixels);
}

int imp::Window::Initialize(const std::string& name, int width, int height)
{
	if (glfwInit() == GLFW_FALSE)
		return -1;
	
	m_Width = width;
	m_Height = height;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_WindowPtr = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);


	glfwSetWindowUserPointer(m_WindowPtr, this);

	LoadLogo(m_WindowPtr);

#if !BENCHMARK_MODE
	ImGui_ImplGlfw_InitForVulkan(m_WindowPtr, true);
#endif
	return 1;
}

void imp::Window::Update()
{
	glfwPollEvents();
}

std::vector<const char*> imp::Window::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	assert(glfwExtensionCount);

	return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

GLFWwindow* const imp::Window::GetWindowPtr() const
{
	return m_WindowPtr;
}

int imp::Window::GetWidth() const
{
	return m_Width;
}

int imp::Window::GetHeight() const
{
	return m_Height;
}

void imp::Window::UpdateDeltaTime()
{
	float now = glfwGetTime();
	m_DeltaTime = now - m_LastTime;
	m_LastTime = now;
}

void imp::Window::DisplayFrameInfo() const
{

	char buffer[100];
	float dtms = static_cast<float>(m_DeltaTime) * 10000;// a bit less than mili
	int fps = 1 / (((dtms > 1) ? floor(dtms) : 1.0f) / 10000.0f);
	snprintf(buffer, 100, "[Debug Frame Info] length: %.3fms FPS: %.4d\0", dtms / 10.0f, fps);

	glfwSetWindowTitle(m_WindowPtr, buffer);
}

void imp::Window::UpdateImGUI()
{
	ImGui_ImplGlfw_NewFrame();
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

