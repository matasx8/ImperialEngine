#pragma once
#include <cstdint>
#include <vcruntime_string.h>
#include <vulkan.h>
#include <vector>
#include <backend/graphics/Surface.h>
#include "backend/graphics/Framebuffer.h"
#include "backend/graphics/CommandBuffer.h"

namespace imp
{
	enum AttachmentLoadOp : uint8_t
	{
		kLoadOpLoad,
		kLoadOpClear,
		kLoadOpDontCare
	};

	enum AttachmentStoreOp : uint8_t
	{
		kStoreOpStore,
		kStoreOpDontCare
	};

	struct RenderPassDesc
	{
		uint8_t msaaCount;
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

	class Graphics;

	class RenderPassBase
	{
	public: 
		RenderPassBase();

		void Create(VkDevice device, RenderPassDesc& desc, std::vector<SurfaceDesc>& surfaceDescs, std::vector<SurfaceDesc>& resolveDescs);

		virtual void Execute(Graphics& gfx) = 0;

		bool HasBackbuffer() const;
		const std::vector<SurfaceDesc>& GetSurfaceDescriptions() const;
		const std::vector<SurfaceDesc>& GetResolveSurfaceDescriptions() const;
		VkRenderPass GetVkRenderPass() const;
		RenderPassDesc GetRenderPassDesc() const;
		std::vector<VkSemaphore> GetSemaphoresToWaitOn();
		std::vector<Surface> GiveSurfaces();

		void Destroy(VkDevice device);

	protected:

		void BeginRenderPass(Graphics& gfx, CommandBuffer cmb);
		void EndRenderPass(Graphics& gfx, CommandBuffer cmb);

		std::vector<VkAttachmentDescription> CreateAttachmentDescs(const RenderPassDesc& desc, const std::vector<SurfaceDesc>& surfaceDescs) const;
		std::vector<VkAttachmentDescription> CreateResolveAttachmentDescs(const RenderPassDesc& desc, const std::vector<SurfaceDesc>& surfaceDescs) const;

		VkRenderPass m_RenderPass;
		std::vector<SurfaceDesc> m_SurfaceDescriptions;
		std::vector<SurfaceDesc> m_ResolveDescriptions;
		RenderPassDesc m_Desc;
		Framebuffer m_Framebuffer;

		std::vector<Surface> m_Surfaces;
	};
}