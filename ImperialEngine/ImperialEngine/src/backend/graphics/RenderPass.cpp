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

	const auto bigVtxBuffer = gfx.m_VertexBuffer.GetBuffer();
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, &bigVtxBuffer, offsets);

	const auto bigIdxBuffer = gfx.m_IndexBuffer.GetBuffer();
	vkCmdBindIndexBuffer(cb, bigIdxBuffer, 0, VK_INDEX_TYPE_UINT32);

	for (const auto& drawData : gfx.m_DrawData) // seems like we might something useful for draw indirect?
	{
		// if material of old drawData is not same
		// then get pipeline of new material + renderpass
		const auto pipe = gfx.EnsurePipeline(cb, *this);	// since we have to get pipeline, bind here and not in that func

		pushData[0] = drawData.Transform;
		gfx.PushConstants(cb, pushData.data(), sizeof(pushData), pipe.GetPipelineLayout());

		//const auto indexCount = gfx.BindMesh(cb, drawData.VertexBufferId);
		const auto mesh = gfx.m_VertexBuffers.find(drawData.VertexBufferId)->second;
		vkCmdDrawIndexed(cb, mesh.indices.GetCount(), 1, mesh.indices.GetOffset(), mesh.vertices.GetOffset(), 0);
		//gfx.DrawIndexed(cb, indexCount);
	}

	EndRenderPass(gfx, cmb);
	cmb.End();
	// TODOOO: seems like we have a race condition. We don't wait for upload command buffer for vertices and indices to finish..
	// that's probbaly why it crashed with big suzanne
	// possible solution is to have vulkan resources have a semaphore member

	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
