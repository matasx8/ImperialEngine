#pragma once
#include <cstdint>
#include <string>
#include <vulkan.h>
#include <vector>
#include <array>
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

	inline constexpr uint32_t kMaxColorAttachmentCount = 4;

	struct RenderPassDesc
	{
		std::array<SurfaceDesc, kMaxColorAttachmentCount> colorSurfaces;
		std::array<SurfaceDesc, kMaxColorAttachmentCount> resolveSurfaces;
		SurfaceDesc depthSurface;
		uint8_t colorAttachmentCount;

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

		void Create(VkDevice device, const RenderPassDesc& desc);

		virtual void Execute(Graphics& gfx) = 0;

		bool HasBackbuffer() const;
		const std::array<SurfaceDesc, kMaxColorAttachmentCount>& GetSurfaceDescriptions() const;
		const std::array<SurfaceDesc, kMaxColorAttachmentCount>& GetResolveSurfaceDescriptions() const;
		VkRenderPass GetVkRenderPass() const;
		RenderPassDesc GetRenderPassDesc() const;
		VkViewport GetViewport() const;
		VkRect2D GetScissor() const;
		std::vector<VkSemaphore> GetSemaphoresToWaitOn();
		std::vector<Surface> GiveSurfaces();

		void Destroy(VkDevice device);

	protected:

		void BeginRenderPass(Graphics& gfx, CommandBuffer cmb);
		void EndRenderPass(Graphics& gfx, CommandBuffer cmb);

		std::vector<VkAttachmentDescription> CreateAttachmentDescs(const SurfaceDesc* descs, const uint32_t descCount) const;
		std::vector<VkAttachmentDescription> CreateResolveAttachmentDescs(const SurfaceDesc* descs, const uint32_t descCount) const;

		VkRenderPass m_RenderPass;
		RenderPassDesc m_Desc;
		Framebuffer m_Framebuffer;

		std::vector<Surface> m_Surfaces;
	};
}