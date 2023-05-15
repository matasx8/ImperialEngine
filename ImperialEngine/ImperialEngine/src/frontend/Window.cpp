#include "frontend/Window.h"
#include "extern/GLFW/glfw3.h"
#include "Debug.h"
#include "extern/IMGUI/backends/imgui_impl_glfw.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "extern/STB/stb_image.h"
#include "Utils/EngineStaticConfig.h"
#include <assert.h>
#include <GLM/ext/matrix_float4x4.hpp>
#include <GLM/ext/matrix_transform.hpp>

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

#if !BENCHMARK_MODE
    glfwMaximizeWindow(m_WindowPtr);
#endif


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

void imp::Window::MoveCameraWithControls(glm::mat4x4& transform)
{
    // Define the translation speed
    float translationSpeed = 10.0f; // units per second

    // Get the current time and the time of the last frame
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;

    // Get the current state of the 'W' key
    int wKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_W);

    // If the 'W' key is pressed, translate the transform matrix forward
    if (wKeyState == GLFW_PRESS)
    {
        glm::vec3 forward(0.0f, 0.0f, -1.0f);
        glm::vec3 translation = forward * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }

    // Get the current state of the 'S' key
    int sKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_S);

    // If the 'S' key is pressed, translate the transform matrix backward
    if (sKeyState == GLFW_PRESS)
    {
        glm::vec3 backward(0.0f, 0.0f, 1.0f);
        glm::vec3 translation = backward * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }

    // Get the current state of the 'A' key
    int aKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_A);

    // If the 'A' key is pressed, translate the transform matrix left
    if (aKeyState == GLFW_PRESS)
    {
        glm::vec3 left(1.0f, 0.0f, 0.0f);
        glm::vec3 translation = left * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }

    // Get the current state of the 'D' key
    int dKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_D);

    // If the 'D' key is pressed, translate the transform matrix right
    if (dKeyState == GLFW_PRESS)
    {
        glm::vec3 right(-1.0f, 0.0f, 0.0f);
        glm::vec3 translation = right * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }

    // Get the current state of the space bar key
    int spaceKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_SPACE);

    // If the space bar key is pressed, translate the transform matrix up
    if (spaceKeyState == GLFW_PRESS)
    {
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 translation = up * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }

    // Get the current state of the space bar key
    int shiftKeyState = glfwGetKey(m_WindowPtr, GLFW_KEY_B);

    // If the space bar key is pressed, translate the transform matrix up
    if (shiftKeyState == GLFW_PRESS)
    {
        glm::vec3 up(0.0f, -1.0f, 0.0f);
        glm::vec3 translation = up * deltaTime * translationSpeed;
        transform = glm::translate(transform, translation);
    }
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

