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
	CommandBuffer cmb = cmbs[0];
	VkCommandBuffer cb = cmb.cmb;
	cmb.Begin();
	BeginRenderPass(gfx, cmb);

	// since 

	for (const auto& drawData : gfx.m_DrawData)
	{
		// if material of old drawData is not same
		// then get pipeline of new material + renderpass
		// gfx.EnsurePipeline(cb, drawData.material, *this) or something like that
		gfx.BindMesh(cb, drawData.VertexBufferId);
		gfx.PushConstants(cb, &drawData.Transform, sizeof(drawData.Transform), 0);
	}

	EndRenderPass(gfx, cmb);
	cmb.End();
	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
