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

	const auto dset = gfx.m_ShaderManager.GetDescriptorSet(gfx.m_Swapchain.GetFrameClock());
	const auto pipe = gfx.EnsurePipeline(cb, *this);	// since we have to get pipeline, bind here and not in that func
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.GetPipelineLayout(), 0, 1, &dset, 0, nullptr);


	// !HERE: global is done. Now need to create draw data!
	std::array<uint32_t, 1> pushData;
	pushData[0] = kDefaultMaterialIndex;

	const auto bigVtxBuffer = gfx.m_VertexBuffer.GetBuffer();
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, &bigVtxBuffer, offsets);

	const auto bigIdxBuffer = gfx.m_IndexBuffer.GetBuffer();
	vkCmdBindIndexBuffer(cb, bigIdxBuffer, 0, VK_INDEX_TYPE_UINT32);

	uint32_t drawIndex = 0;
	for (const auto& drawData : gfx.m_DrawData) // seems like we might something useful for draw indirect?
	{
		// if material of old drawData is not same
		// then get pipeline of new material + renderpass

		// since all pipelines will have same layout this is should be way above

		gfx.PushConstants(cb, &drawIndex, sizeof(uint32_t), pipe.GetPipelineLayout());

		//const auto indexCount = gfx.BindMesh(cb, drawData.VertexBufferId);
		const auto mesh = gfx.m_VertexBuffers.find(drawData.VertexBufferId)->second;
		vkCmdDrawIndexed(cb, mesh.indices.GetCount(), 1, mesh.indices.GetOffset(), mesh.vertices.GetOffset(), 0);
		drawIndex++;
	}

	EndRenderPass(gfx, cmb);
	cmb.End();
	
	// since we know vertex and index buffer will have same semaphore it's safe to do this for now
	// but probably should implement some easier way to round up all unique semaphores.. Or is it not needed? Maybe we can wait of two identical semaphores
	auto semaphores = GetSemaphoresToWaitOn();
	if(gfx.m_VertexBuffer.HasSemaphore())
		semaphores.push_back(gfx.m_VertexBuffer.StealSemaphore());

	gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, GetSemaphoresToWaitOn());
}
