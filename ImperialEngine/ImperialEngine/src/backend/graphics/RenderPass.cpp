#include "RenderPass.h"
#include "backend/graphics/Graphics.h"
#include "GLM/gtc/matrix_transform.hpp"

imp::RenderPass::RenderPass()
	: RenderPassBase()
{
}

void imp::RenderPass::Execute(Graphics& gfx, const CameraData& cam)
{
	constexpr uint32_t numCmbs = 1;
	auto cmbs = gfx.m_CbManager.AquireCommandBuffers(gfx.m_LogicalDevice, numCmbs);
	CommandBuffer cmb = cmbs[0];
	VkCommandBuffer cb = cmb.cmb;
	cmb.Begin();
	BeginRenderPass(gfx, cmb);

	std::array<glm::mat4x4, 2> pushData;
	pushData[1] = cam.Projection * cam.View;

	for (const auto& drawData : gfx.m_DrawData)
	{
		// if material of old drawData is not same
		// then get pipeline of new material + renderpass
		const auto pipe = gfx.EnsurePipeline(cb, *this);	// since we have to get pipeline, bind here and not in that func

		pushData[0] = drawData.Transform;
		gfx.PushConstants(cb, pushData.data(), sizeof(pushData), pipe.GetPipelineLayout());

		const auto indexCount = gfx.BindMesh(cb, drawData.VertexBufferId);
		gfx.DrawIndexed(cb, indexCount);
	}

	EndRenderPass(gfx, cmb);
	cmb.End();
	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
