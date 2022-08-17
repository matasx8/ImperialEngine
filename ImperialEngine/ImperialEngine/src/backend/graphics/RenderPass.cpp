#include "RenderPass.h"
#include "backend/graphics/Graphics.h"

imp::RenderPass::RenderPass()
	: RenderPassBase()
{
}

void imp::RenderPass::Execute(Graphics& gfx)
{
	constexpr uint32_t numCmbs = 1;
	auto cmbs = gfx.m_CbManager.AquireCommandBuffers(gfx.m_LogicalDevice, numCmbs);
	auto cmb = cmbs[0];
	cmb.Begin();
	BeginRenderPass(gfx, cmb);

	// !HERE I now have vertex buffers, to draw I need a shader.. carry on with that

	EndRenderPass(gfx, cmb);
	cmb.End();
	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
