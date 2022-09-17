#include "RenderPassBase.h"
#include "backend/graphics/Graphics.h"
#include <stdexcept>
#include <cassert>

imp::RenderPassBase::RenderPassBase()
	: m_RenderPass(), m_Desc(), m_Framebuffer(0)
{
}

void imp::RenderPassBase::Create(VkDevice device, const RenderPassDesc& desc)
{
	m_Desc = desc;

	auto attachmentDescriptions = CreateAttachmentDescs(m_Desc.colorSurfaces.data(), m_Desc.colorAttachmentCount);
	if (desc.depthSurface.format)
	{
		auto depthDescriptions = CreateAttachmentDescs(&m_Desc.depthSurface, 1);
		attachmentDescriptions.insert(attachmentDescriptions.end(), depthDescriptions.begin(), depthDescriptions.end());
	}
	// TODO: not implemented!! supposed to be empty rn
	auto resolveDescriptions = CreateResolveAttachmentDescs(m_Desc.colorSurfaces.data(), m_Desc.colorAttachmentCount);

	std::vector<VkAttachmentReference> colorAttachments;
	colorAttachments.resize(desc.colorAttachmentCount);
	for (int i = 0; i < desc.colorAttachmentCount; i++)
	{
		colorAttachments[i].attachment = i;
		colorAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depthAttachmentReference;
	if (desc.depthSurface.format)
	{
		depthAttachmentReference.attachment = desc.colorAttachmentCount; // depth is indexed after color
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// TODO: only works with one
	assert(resolveDescriptions.size() <= 1);
	VkAttachmentReference colorAttachmentResolveReference;
	if (resolveDescriptions.size())
	{
		assert(desc.resolveSurfaces[0].msaaCount > 1);
		if (desc.depthSurface.format)
			colorAttachmentResolveReference.attachment = desc.colorAttachmentCount + 1;
		else
			colorAttachmentResolveReference.attachment = desc.colorAttachmentCount;
		colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (desc.resolveSurfaces[0].msaaCount > 1)
		attachmentDescriptions.push_back(std::move(resolveDescriptions[0]));

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = desc.colorAttachmentCount;
	subpass.pColorAttachments = desc.colorAttachmentCount ? colorAttachments.data() : nullptr;
	subpass.pDepthStencilAttachment = desc.depthSurface.format ? &depthAttachmentReference : nullptr;
	subpass.pResolveAttachments = desc.resolveSurfaces[0].msaaCount > 1 ? &colorAttachmentResolveReference : nullptr;

	// so far we have 1 subpass supported so leave this
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstAccessMask = 0;


	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &m_RenderPass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create a Render Pass");
}

bool imp::RenderPassBase::HasBackbuffer() const
{
	throw std::runtime_error("Not implemented");
	return false;
}

const std::array<imp::SurfaceDesc, imp::kMaxColorAttachmentCount>& imp::RenderPassBase::GetSurfaceDescriptions() const
{
	return m_Desc.colorSurfaces;
}

const std::array<imp::SurfaceDesc, imp::kMaxColorAttachmentCount>& imp::RenderPassBase::GetResolveSurfaceDescriptions() const
{
	return m_Desc.resolveSurfaces;
}

VkRenderPass imp::RenderPassBase::GetVkRenderPass() const
{
	return m_RenderPass;
}

imp::RenderPassDesc imp::RenderPassBase::GetRenderPassDesc() const
{
	return m_Desc;
}

VkViewport imp::RenderPassBase::GetViewport() const
{
	VkViewport vp;
	vp.x = 0.0f;
	vp.y = 0.0f;
	vp.width = GetSurfaceDescriptions().front().width;
	vp.height = GetSurfaceDescriptions().front().height;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	assert(vp.width && vp.height);
	return vp;
}

VkRect2D imp::RenderPassBase::GetScissor() const
{
	VkRect2D rect;
	rect.offset = {};
	rect.extent = { GetSurfaceDescriptions().front().width, GetSurfaceDescriptions().front().height };
	assert(rect.extent.width && rect.extent.height);
	return rect;
}

std::vector<VkSemaphore> imp::RenderPassBase::GetSemaphoresToWaitOn()
{
	std::vector<VkSemaphore> sems;
	for (auto& surf : m_Surfaces)
	{
		auto sem = surf.GetSemaphore();
		if(sem)
			sems.push_back(surf.GetSemaphore());
	}
	return sems;
}

std::vector<imp::Surface> imp::RenderPassBase::GiveSurfaces()
{
	std::vector<imp::Surface> surfaceCopies;
	for (auto&& surf : m_Surfaces)
	{
		if(!surf.GetDesc().isBackbuffer)	// swapchain images are controlled by the swapchain
			surfaceCopies.emplace_back(surf);
	}
	return surfaceCopies;
}

void imp::RenderPassBase::Destroy(VkDevice device)
{
	m_Framebuffer.Destroy(device);
	vkDestroyRenderPass(device, m_RenderPass, nullptr);
}

void imp::RenderPassBase::BeginRenderPass(Graphics& gfx, CommandBuffer cmb)
{
	// TODO: should somehow be able to get input surfaces
	// and also attachments and map them to the surface descriptions
	auto& surfaces = m_Surfaces;
	if (surfaces.size())
	{
		gfx.m_SurfaceManager.ReturnSurfaces(surfaces);
		surfaces = {};
	}
	for (const auto& surfDesc : GetSurfaceDescriptions())
	{
		if (surfDesc.format)
		{
			if (surfDesc.isBackbuffer)
				surfaces.push_back(gfx.m_Swapchain.GetSwapchainImageSurface(gfx.m_LogicalDevice));
			else
				surfaces.push_back(gfx.m_SurfaceManager.GetSurface(surfDesc, gfx.m_LogicalDevice)); // TODO: change to emplace?
		}
	}
	if(m_Desc.depthSurface.format)
		surfaces.push_back(gfx.m_SurfaceManager.GetSurface(m_Desc.depthSurface, gfx.m_LogicalDevice));
	// check if framebuffer we have still is good
	if (!m_Framebuffer.StillValid(surfaces))
	{
		if(m_Framebuffer.GetVkFramebuffer()) // TODO: this is a fast work around to not destroy an empty frame buffer
			gfx.m_VulkanGarbageCollector.AddGarbageResource(std::make_shared<Framebuffer>(m_Framebuffer));
		m_Framebuffer = gfx.m_SurfaceManager.CreateFramebuffer(*this, surfaces, gfx.m_LogicalDevice);
	}
	m_Framebuffer.UpdateLastUsed(gfx.m_CurrentFrame);
	
	// render pass should hold info for clears
	// TODO: make the clear dynamic
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 1.0f, 0.0f, 0.0f };
	clearValues[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { GetSurfaceDescriptions()[0].width, GetSurfaceDescriptions()[0].height};
	// temp way to know if clear
	bool clear = (m_Desc.colorSurfaces[0].loadOp == kLoadOpClear || m_Desc.depthSurface.loadOp == kLoadOpClear);
	renderPassBeginInfo.pClearValues = clear ? clearValues.data() : nullptr;
	renderPassBeginInfo.clearValueCount = clear ? clearValues.size() : 0;
	renderPassBeginInfo.framebuffer = m_Framebuffer.GetVkFramebuffer();

	vkCmdBeginRenderPass(cmb.cmb, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void imp::RenderPassBase::EndRenderPass(Graphics& gfx, CommandBuffer cmb)
{
	vkCmdEndRenderPass(cmb.cmb);
}

std::vector<VkAttachmentDescription> imp::RenderPassBase::CreateAttachmentDescs(const SurfaceDesc* descs, const uint32_t descCount) const
{
	auto attDescs = std::vector<VkAttachmentDescription>();

	for(auto i = 0; i < descCount; i++)
	{
		const auto& descr = descs[i];
		VkAttachmentDescription attDesc = {};
		attDesc.flags = 1; // TODO: what are these?
		attDesc.format = descr.format;
		attDesc.samples = static_cast<VkSampleCountFlagBits>(descr.msaaCount);
		attDesc.loadOp = static_cast<VkAttachmentLoadOp>(descr.loadOp);
		attDesc.storeOp = static_cast<VkAttachmentStoreOp>(descr.storeOp);
		attDesc.initialLayout = static_cast<VkImageLayout>(descr.initialLayout);
		attDesc.finalLayout = static_cast<VkImageLayout>(descr.finalLayout);

		attDescs.emplace_back(attDesc);
	}
	return attDescs;
}

std::vector<VkAttachmentDescription> imp::RenderPassBase::CreateResolveAttachmentDescs(const SurfaceDesc* descs, const uint32_t descCount) const
{
	auto attDescs = std::vector<VkAttachmentDescription>();

	//for (auto& descr : surfaceDescs)
	//{
	//	VkAttachmentDescription attDesc = {};
	//	attDesc.flags = 1;
	//	attDesc.format = descr.format;
	//	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	//	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//	attDesc.finalLayout = static_cast<VkImageLayout>(descr.finalLayout);

	//	descs.emplace_back(attDesc);
	//}
	// TODO: not implemented
	return attDescs;
}
