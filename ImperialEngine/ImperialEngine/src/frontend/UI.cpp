#include "UI.h"

#include <GLM/trigonometric.hpp>
#include <GLM/gtx/quaternion.hpp>
#include "Components/Components.h"
#include <imgui.h>
#include <extern/IMGUI/imGuIZMOquat.h>

namespace imp
{
	UI::UI()
		: m_ShowDefaultWindow(false)
	{
	}

	void UI::Initialize()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		m_ShowDefaultWindow = true;
	}

	void UI::Update(entt::registry& reg)
	{
		ImGui::NewFrame();
//#define demolol
#ifdef demolol
		ImGui::ShowDemoWindow();
#else

		const auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
		ImGui::Begin("Default Window", &m_ShowDefaultWindow, flags);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("Tabs", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Inspector"))
			{
				ImGui::Text("This is going to be the inspector. Will show last selected item");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Profiling"))
			{
				ImGui::Text("This is going to be profiling data");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Cameras"))
			{
				ImGui::Text("lol camera");
				const auto cameras = reg.view<Comp::Transform, Comp::Camera>();
				for (auto ent : cameras)
				{
					auto& transform = cameras.get<Comp::Transform>(ent);
					auto& cam = cameras.get<Comp::Camera>(ent);

					auto& pos = transform.GetPosition();
					ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pos), 0.1f, -99999999999999.0f, 99999999999999.0f);

					auto quat = glm::toQuat(transform.transform);
					glm::mat4 rot;
					ImGui::Text("Camera Orientation");
					if (ImGui::gizmo3D("##gizmo2", quat, (ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x) * 24.0f))
					{
						rot = glm::toMat4(quat);
						const glm::vec3 poss = transform.GetPosition();
						transform.transform = glm::translate(glm::mat4x4(1.0f), poss);
						transform.transform *= rot;
					}

					// we can find the FOV from the projection matrix
					int fov = 2.0f * glm::atan(1.0f / cam.projection[1][1]) * 180.0f / glm::pi<float>();
					float aspect = cam.projection[1][1] / cam.projection[0][0];
					if (ImGui::DragInt("FOV", &fov, 1.0f, 1, 180))
					{
						cam.projection = glm::perspective(glm::radians((float)fov), aspect, 0.1f, 10000.0f);
					}
					
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
#endif
		ImGui::Render();
	}

	void UI::Destroy()
	{
		ImGui::DestroyContext();
	}
}