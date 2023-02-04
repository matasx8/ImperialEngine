#include "UI.h"
#include "frontend/Engine.h"
#include <GLM/trigonometric.hpp>
#include <GLM/gtx/quaternion.hpp>
#include "Components/Components.h"
#include <imgui.h>
#include <extern/IMGUI/imGuIZMOquat.h>
#include <sstream>

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

	void UI::Update(Engine& engine, entt::registry& reg)
	{
		ImGui::NewFrame();
//#define SHOW_DEMO
//#ifdef SHOW_DEMO
//		ImGui::ShowDemoWindow();
//#else

		const auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
		ImGui::Begin("Imperial Settings", &m_ShowDefaultWindow, flags);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("Tabs", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Inspector"))
			{
				ImGui::Text("This is going to be the inspector. Will show last selected item");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Utilities"))
			{
				ImGui::Text("Various utilities..");
				static int clicked = 0;
				if (ImGui::Button("Add random meshes"))
					engine.AddDemoEntity(1);
				if (ImGui::Button("Add A LOT of random meshes (10k)"))
					engine.AddDemoEntity(10000);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Profiling"))
			{
				ImGui::Text("This is going to be profiling data");
				constexpr ImVec4 mainThreadCol(1.0f, 1.0f, 0.0f, 1.0f);
				constexpr ImVec4 renderThreadCol(0.0f, 1.0f, 0.0f, 1.0f);
				constexpr ImVec4 syncCol(1.0f, 1.0f, 1.0f, 1.0f);

				const auto& timings = engine.GetFrameTimings();
				const auto& syncTimings = engine.GetSyncTimings();
				const auto& gfxTimings = engine.GetGfxFrameTimings();
				const auto& gfxSyncTimings = engine.GetGfxSyncTimings();

				std::stringstream ss1;
				std::stringstream ss2;
				std::stringstream ss3;
				std::stringstream ss4;
				std::stringstream ss5;
				std::stringstream ss6;
				std::stringstream ss7;
				std::stringstream ss8;

				ss1 << "Main Thread time spent working on frame 'n': ";
				ss1 << timings.frameWorkTime.ms();
				ss2 << "Main Thread time spent waiting for Render Thread 'n': ";
				ss2 << timings.waitTime.ms();
				ss3 << "Main Thread total time spent working and waiting on frame 'n': ";
				ss3 << timings.totalFrameTime.ms();
				ss4 << "Time spent executing sync function: ";
				ss4 << syncTimings.ms();
				ss5 << "Render Thread time spent working on frame 'n': ";
				ss5 << gfxTimings.frameWorkTime.ms();
				ss6 << "Render Thread time spent waiting for Main Thread 'n': ";
				ss6 << gfxTimings.waitTime.ms();
				ss7 << "Render Thread total time spent working and waiting on frame 'n': ";
				ss7 << gfxTimings.totalFrameTime.ms();
				ss8 << "Render Thread time spent waiting for GPU: ";
				ss8 << gfxSyncTimings.ms();

				ImGui::TextColored(mainThreadCol, ss1.str().c_str());
				ImGui::TextColored(mainThreadCol, ss2.str().c_str());
				ImGui::TextColored(mainThreadCol, ss3.str().c_str());
				ImGui::TextColored(syncCol, ss4.str().c_str());
				ImGui::TextColored(renderThreadCol, ss5.str().c_str());
				ImGui::TextColored(renderThreadCol, ss6.str().c_str());
				ImGui::TextColored(renderThreadCol, ss7.str().c_str());
				ImGui::TextColored(renderThreadCol, ss8.str().c_str());

				// there are some other places we're waiting for gpu
				// could add time spent working and not waiting

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Camera"))
			{
				const auto cameras = reg.view<Comp::Transform, Comp::Camera>();
				for (auto ent : cameras)
				{
					auto& transform = cameras.get<Comp::Transform>(ent);
					auto& cam = cameras.get<Comp::Camera>(ent);
					auto& pos = transform.GetPosition();

					ImGui::Text("Select Rendering Mode:");
					static int renderItemSelected = static_cast<int>(kDefaultEngineRenderMode);
					if (ImGui::Combo("Rendering Mode", &renderItemSelected, "Traditional CPU-Driven\0GPU-Driven\0GPU-Driven Mesh Shading"))
					{
						engine.SwitchRenderingMode(static_cast<EngineRenderMode>(renderItemSelected));
					}

					ImGui::Text("Select Camera Output [not implemeted yet]:");
					static int itemSelected = 0;
					if (ImGui::Combo("C/O", &itemSelected, "Color Framebuffer\0Depth Framebuffer\0"))
					{
						cam.camOutputType = static_cast<uint32_t>(itemSelected) + 1u;
						cam.dirty = true;
					}

					ImGui::Text("Camera Position:");
					ImGui::DragFloat3("POS", reinterpret_cast<float*>(&pos), 0.1f, -99999999999999.0f, 99999999999999.0f);

					ImGui::Text("Camera Orientation");
					auto quat = glm::toQuat(transform.transform);
					glm::mat4 rot;
					if (ImGui::gizmo3D("##gizmo2", quat, (ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x) * 24.0f, 0x0200))
					{
						rot = glm::toMat4(quat);
						const glm::vec3 poss = transform.GetPosition();
						transform.transform = glm::translate(glm::mat4x4(1.0f), poss);
						transform.transform *= rot;
					}

					bool fovChanged = false, nearFarChanged = false;
					int fov = 2.0f * glm::atan(1.0f / cam.projection[1][1]) * 180.0f / glm::pi<float>();

					// TODO: figure out how to get near and far value from projection matrix
					// currently if nearAndFar is not in sync with what the camera is initialized with then
					// after changing fov or nearfar param it might jump unpleasantly
					static float nearAndFar[2] = { 5.0f, 1000.0f };
					ImGui::Text("Camera FOV:");
					if (ImGui::DragInt("FOV", &fov, 1.0f, 1, 180)) fovChanged = true;
					ImGui::Text("Camera Near And Far:");
					if (ImGui::DragFloat2("N/F", nearAndFar, 0.5f, 0.01f, 10000.0f)) nearFarChanged = true;

					if (nearFarChanged || fovChanged)
					{
						const float aspect = cam.projection[1][1] / cam.projection[0][0];

						cam.projection = glm::perspective(glm::radians((float)fov), aspect, nearAndFar[0], nearAndFar[1]);
					}
					
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
//#endif
		ImGui::Render();
	}

	void UI::Destroy()
	{
		ImGui::DestroyContext();
	}
}