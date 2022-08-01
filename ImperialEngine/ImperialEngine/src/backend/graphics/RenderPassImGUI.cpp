#include "RenderPassImGUI.h"
#include "backend/graphics/Graphics.h"
#include <imgui.h>
#include <IMGUI/backends/imgui_impl_vulkan.h>

imp::RenderPassImGUI::RenderPassImGUI()
{

}

void imp::RenderPassImGUI::Execute(Graphics& gfx)
{
	constexpr uint32_t numCmbs = 1;
	auto cmbs = gfx.m_CbManager.AquireCommandBuffers(gfx.m_LogicalDevice, numCmbs);
	auto cmb = cmbs[0];
	cmb.Begin();
	BeginRenderPass(gfx, cmb);

	ImGui::NewFrame();
	bool ye = true;
	ImGui::ShowDemoWindow(&ye);
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmb.cmb);
	EndRenderPass(gfx, cmb);
	cmb.End();
	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
