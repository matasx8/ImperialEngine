#pragma once
#include <string>
#include <vector>

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

		bool ShouldClose() const;

		void Close();
	private:
		int m_Width, m_Height;
		GLFWwindow* m_WindowPtr;
	};
}
