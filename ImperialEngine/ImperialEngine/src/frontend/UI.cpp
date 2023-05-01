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

	void UI::Update(Engine& engine, entt::registry& reg, CircularFrameTimeRowContainer& stats)
	{
#if !BENCHMARK_MODE
		ImGui::NewFrame();
//#define SHOW_DEMO 1
#if SHOW_DEMO
		ImGui::ShowDemoWindow();
#else
		const auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
		ImGui::Begin("Imperial Settings", &m_ShowDefaultWindow, flags);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("Tabs", tab_bar_flags))
		{
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
				const auto maxScale = stats.GetMaximumValues();
				const auto avg = stats.GetAverageValues();
				char overlay[32];
				sprintf_s(overlay, "avg %.3f ms", avg.frameMainCPU);
				ImGui::PlotHistogram("Main Thread CPU", &stats.data()->frameMainCPU, stats.size(), 0, overlay, 0.0f, maxScale.frameMainCPU * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));

				if (avg.triangles < 1e+3)
					sprintf_s(overlay, "avg %llu tris", static_cast<uint64_t>(avg.triangles));
				else if (avg.triangles < 1e+6)
					sprintf_s(overlay, "avg %lluk tris", static_cast<uint64_t>(avg.triangles / 1e+3f));
				else if (avg.triangles < 1e+9)
					sprintf_s(overlay, "avg %lluM tris", static_cast<uint64_t>(avg.triangles / 1e+6f));
				else
					sprintf_s(overlay, "avg. %lluB tris", static_cast<uint64_t>(avg.triangles / 1e+9f));

				ImGui::PlotHistogram("Triangles", &stats.data()->triangles, stats.size(), 0, overlay, 0.0f, maxScale.triangles * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));
				
				sprintf_s(overlay, "avg %.3f ms", avg.frame);
				ImGui::PlotHistogram("Frame Time", &stats.data()->frame, stats.size(), 0, overlay, 0.0f, maxScale.frame * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));

				sprintf_s(overlay, "avg %.3f ms", avg.frameRenderCPU);
				ImGui::PlotHistogram("Render Thread CPU", &stats.data()->frameRenderCPU, stats.size(), 0, overlay, 0.0f, maxScale.frameRenderCPU * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));

				sprintf_s(overlay, "avg %.3f ms", avg.frameGPU);
				ImGui::PlotHistogram("GPU Frame Time", &stats.data()->frameGPU, stats.size(), 0, overlay, 0.0f, maxScale.frameGPU * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));
				
				sprintf_s(overlay, "avg %.3f ms", avg.cull);
				ImGui::PlotHistogram("Cull Time", &stats.data()->cull, stats.size(), 0, overlay, 0.0f, maxScale.cull * 2.0f, ImVec2(0, 80.0f), sizeof(FrameTimeRow));



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
					if (cam.preview)
						continue;

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
			if (ImGui::BeginTabItem("Preview Camera"))
			{
				const auto cameras = reg.view<Comp::Transform, Comp::Camera>();
				for (auto ent : cameras)
				{
					auto& transform = cameras.get<Comp::Transform>(ent);
					auto& cam = cameras.get<Comp::Camera>(ent);
					if (!cam.preview)
						continue;
					auto& pos = transform.GetPosition();

					static bool usePreviewCam = false;
					if (ImGui::Checkbox("Use Preview Camera", &usePreviewCam))
					{
						cam.isRenderCamera = usePreviewCam;
					}

					ImGui::Text("Camera Position:");
					ImGui::DragFloat3("PPOS", reinterpret_cast<float*>(&pos), 0.1f, -99999999999999.0f, 99999999999999.0f);

					ImGui::Text("Camera Orientation");
					auto quat = glm::toQuat(transform.transform);
					glm::mat4 rot;
					if (ImGui::gizmo3D("##gizmo3", quat, (ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x) * 24.0f, 0x0200))
					{
						rot = glm::toMat4(quat);
						const glm::vec3 poss = transform.GetPosition();
						transform.transform = glm::translate(glm::mat4x4(1.0f), poss);
						transform.transform *= rot;
					}

					bool fovChanged = false, nearFarChanged = false;
					int fov = 2.0f * glm::atan(1.0f / cam.projection[1][1]) * 180.0f / glm::pi<float>();

					static float nearAndFar[2] = { 5.0f, 1000.0f };
					ImGui::Text("Camera FOV:");
					if (ImGui::DragInt("FOV2", &fov, 1.0f, 1, 180)) fovChanged = true;
					ImGui::Text("Camera Near And Far:");
					if (ImGui::DragFloat2("N/F2", nearAndFar, 0.5f, 0.01f, 10000.0f)) nearFarChanged = true;

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
#endif
		ImGui::Render();
#endif
	}

	void UI::Destroy()
	{
		ImGui::DestroyContext();
	}
}