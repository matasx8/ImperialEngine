#include "UI.h"
#include <imgui.h>

namespace imp
{
	UI::UI()
	{
	}

	void UI::Initialize()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();
	}

	void UI::Update()
	{
		ImGui::NewFrame();
		bool ye = true;
		ImGui::ShowDemoWindow(&ye);
		ImGui::Render();
	}

	void UI::Destroy()
	{
		ImGui::DestroyContext();
	}
}