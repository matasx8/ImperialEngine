#pragma once
#include <cstdint>
#include <vcruntime_string.h>
#include <vulkan.h>
#include <vector>

namespace imp
{
	struct RenderPassDesc
	{
		uint8_t msaaCount;
		uint8_t renderpassPlace;
		//uint8_t framebufferTarget;
		uint8_t colorAttachmentCount;
		uint8_t colorFormat;
		uint8_t colorLoadOp;
		uint8_t colorStoreOp;
		uint8_t depthFormat;
		uint8_t depthLoadOp;
		uint8_t depthStoreOp;

		bool operator==(const RenderPassDesc& other) const noexcept
		{
			return memcmp(this, &other, sizeof(RenderPassDesc));
		}
	};

	class RenderPassBase
	{
	public: 
		RenderPassBase();

		virtual void Create();

		virtual void Execute() = 0;

		virtual void Destroy();

	private:
		VkRenderPass m_RenderPass;
		std::vector<std::pair<uint32_t, SurfaceDesc>> m_SurfaceDescriptions;
		RenderPassDesc m_Desc;
	};
}