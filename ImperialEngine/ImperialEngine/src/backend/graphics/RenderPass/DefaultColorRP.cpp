#include "DefaultColorRP.h"
#include "backend/graphics/Graphics.h"
#include "GLM/gtc/matrix_transform.hpp"
#include <IPROF/iprof.hpp>

namespace imp
{
	DefaultColorRP::DefaultColorRP()
		: RenderPass()
	{
	}

	void DefaultColorRP::Execute(Graphics& gfx, const CameraData& cam)
	{
		IPROF_FUNC;
		constexpr uint32_t numCmbs = 1;
		auto cmbs = gfx.m_CbManager.AquireCommandBuffers(gfx.m_LogicalDevice, numCmbs);
		CommandBuffer cmb = cmbs[0];
		VkCommandBuffer cb = cmb.cmb;
		cmb.Begin();
		BeginRenderPass(gfx, cmb);

		const auto dset = gfx.m_ShaderManager.GetDescriptorSet(gfx.m_Swapchain.GetFrameClock());
		const auto pipe = gfx.EnsurePipeline(cb, *this);	// since we have to get pipeline, bind here and not in that func
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.GetPipelineLayout(), 0, 1, &dset, 0, nullptr);

		const auto bigVtxBuffer = gfx.m_VertexBuffer.GetBuffer();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, &bigVtxBuffer, offsets);

		const auto bigIdxBuffer = gfx.m_IndexBuffer.GetBuffer();
		vkCmdBindIndexBuffer(cb, bigIdxBuffer, 0, VK_INDEX_TYPE_UINT32);

		const auto renderMode = gfx.GetGraphicsSettings().renderMode;

		const auto cpuDrawBuffer = gfx.m_ShaderManager.GetDrawDataBuffer(gfx.m_Swapchain.GetFrameClock());
		if (renderMode == kEngineRenderModeTraditional)
		{
			uint32_t drawIndex = 0;
			for (const auto& drawData : gfx.m_DrawData)
			{
				gfx.PushConstants(cb, &drawIndex, sizeof(uint32_t), pipe.GetPipelineLayout());

				const auto& mesh = gfx.m_VertexBuffers.at(drawData.VertexBufferId);
				vkCmdDrawIndexed(cb, mesh.indices.GetCount(), 1, mesh.indices.GetOffset(), mesh.vertices.GetOffset(), 0);
				drawIndex++;
			}
		}
		else if (renderMode == kEngineRenderModeGPUDriven)
		{
			vkCmdDrawIndexedIndirect(cb, gfx.m_DrawBuffer.GetBuffer(), 0, gfx.m_NumDraws, sizeof(VkDrawIndexedIndirectCommand));
		}

		EndRenderPass(gfx, cmb);
		cmb.End();

		// since we know vertex and index buffer will have same semaphore it's safe to do this for now
		// but probably should implement some easier way to round up all unique semaphores.. Or is it not needed? Maybe we can wait of two identical semaphores
		auto semaphores = GetSemaphoresToWaitOn();
		if (gfx.m_VertexBuffer.HasSemaphore())
			semaphores.push_back(gfx.m_VertexBuffer.StealSemaphore());
		if(gfx.m_DrawBuffer.HasSemaphore())
			semaphores.push_back(gfx.m_DrawBuffer.StealSemaphore());

		// put this in EndRenderPass?
		auto synchs = gfx.m_CbManager.Submit(gfx.m_GfxQueue, gfx.m_LogicalDevice, cmbs, semaphores, kSubmitDontCare, gfx.m_CurrentFrame);
		std::for_each(m_Surfaces.begin(), m_Surfaces.end(), [&](Surface& surf) {surf.AddSemaphore(synchs.semaphore.semaphore); });
	}
}