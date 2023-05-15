#pragma once
#include <string>
#include <vector>
#include "GLM/mat4x4.hpp"

struct GLFWwindow;

namespace imp
{
	class Window
	{
	public:
		Window();

		int Initialize(const std::string& name, int width, int height);
		void Update();
		std::vector<const char*> GetRequiredExtensions();
		GLFWwindow* const GetWindowPtr() const;
		int GetWidth() const;
		int GetHeight() const;
		void UpdateDeltaTime();
		void DisplayFrameInfo() const;
		void UpdateImGUI();

		void MoveCameraWithControls(glm::mat4x4& transform);

		bool ShouldClose() const;

		void Close();
	private:
		int m_Width, m_Height;
		GLFWwindow* m_WindowPtr;
		double m_LastTime;
		double m_DeltaTime;
	};
}
