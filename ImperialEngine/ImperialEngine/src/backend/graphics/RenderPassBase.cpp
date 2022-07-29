#include "RenderPassBase.h"
#include "backend/graphics/Graphics.h"
#include <stdexcept>
#include <cassert>

imp::RenderPassBase::RenderPassBase()
	: m_RenderPass(), m_SurfaceDescriptions(), m_Desc(), m_Framebuffer(0)
{
}

void imp::RenderPassBase::Create(VkDevice device, RenderPassDesc& desc, std::vector<SurfaceDesc>& surfaceDescs, std::vector<SurfaceDesc>& resolveDescs)
{
	m_Desc = desc;
	m_SurfaceDescriptions = surfaceDescs;
	m_ResolveDescriptions = resolveDescs;

	auto attachmentDescriptions = CreateAttachmentDescs(m_Desc, m_SurfaceDescriptions);
	auto resolveDescriptions = CreateResolveAttachmentDescs(m_Desc, m_ResolveDescriptions);

	std::vector<VkAttachmentReference> colorAttachments;
	colorAttachments.resize(desc.colorAttachmentCount);
	for (int i = 0; i < desc.colorAttachmentCount; i++)
	{
		colorAttachments[i].attachment = i;
		colorAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depthAttachmentReference;
	if (desc.depthFormat)
	{
		depthAttachmentReference.attachment = desc.colorAttachmentCount; // depth is indexed after color
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// TODO: only works with one
	assert(resolveDescriptions.size() <= 1);
	VkAttachmentReference colorAttachmentResolveReference;
	if (resolveDescs.size())
	{
		assert(desc.msaaCount > 1);
		if (desc.depthFormat)
			colorAttachmentResolveReference.attachment = desc.colorAttachmentCount + 1;
		else
			colorAttachmentResolveReference.attachment = desc.colorAttachmentCount;
		colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (desc.msaaCount > 1)
		attachmentDescriptions.push_back(std::move(resolveDescriptions[0]));

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = desc.colorAttachmentCount;
	subpass.pColorAttachments = desc.colorAttachmentCount ? colorAttachments.data() : nullptr;
	subpass.pDepthStencilAttachment = desc.depthFormat ? &depthAttachmentReference : nullptr;
	subpass.pResolveAttachments = desc.msaaCount > 1 ? &colorAttachmentResolveReference : nullptr;

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
	bool hasBB = false;
	if (m_SurfaceDescriptions.size())
		hasBB = m_SurfaceDescriptions[0].isBackbuffer;
	if (m_ResolveDescriptions.size())
		hasBB = m_ResolveDescriptions[0].isBackbuffer;
	return hasBB;
}

const std::vector<imp::SurfaceDesc>& imp::RenderPassBase::GetSurfaceDescriptions() const
{
	return m_SurfaceDescriptions;
}

const std::vector<imp::SurfaceDesc>& imp::RenderPassBase::GetResolveSurfaceDescriptions() const
{
	return m_ResolveDescriptions;
}

VkRenderPass imp::RenderPassBase::GetVkRenderPass() const
{
	return m_RenderPass;
}

imp::RenderPassDesc imp::RenderPassBase::GetRenderPassDesc() const
{
	return m_Desc;
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

void imp::RenderPassBase::Destroy(VkDevice device)
{
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
		if (surfDesc.isBackbuffer)
			surfaces.push_back(gfx.m_Swapchain.GetSwapchainImageSurface(gfx.m_LogicalDevice));
		else
			surfaces.push_back(gfx.m_SurfaceManager.GetSurface(surfDesc, gfx.m_LogicalDevice)); // TODO: change to emplace?
	}
	// check if framebuffer we have still is good
	// TODO: make framebuffer system, creating many fbs causes memory leak
	// can't destroy them immediately either
	if (!m_Framebuffer.StillValid(surfaces))
	{
		m_Framebuffer.Destroy(gfx.m_LogicalDevice);
		m_Framebuffer = gfx.m_SurfaceManager.CreateFramebuffer(*this, surfaces, gfx.m_LogicalDevice);
	}
	
	// render pass should hold info for clears
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 1.0f, 0.0f, 0.0f };
	clearValues[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { m_SurfaceDescriptions[0].width, m_SurfaceDescriptions[0].height };
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.framebuffer = m_Framebuffer.GetVkFramebuffer();

	vkCmdBeginRenderPass(cmb.cmb, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


}

void imp::RenderPassBase::EndRenderPass(Graphics& gfx, CommandBuffer cmb)
{
	vkCmdEndRenderPass(cmb.cmb);
}

std::vector<VkAttachmentDescription> imp::RenderPassBase::CreateAttachmentDescs(const RenderPassDesc& desc, const std::vector<SurfaceDesc>& surfaceDescs) const
{
	auto descs = std::vector<VkAttachmentDescription>();

	for (auto& descr : surfaceDescs)
	{
		VkAttachmentDescription attDesc = {};
		attDesc.flags = 1; // TODO: what are these?
		attDesc.format = descr.format;
		attDesc.samples = static_cast<VkSampleCountFlagBits>(descr.msaaCount);
		attDesc.loadOp = static_cast<VkAttachmentLoadOp>(descr.isColor ? desc.colorLoadOp : desc.depthLoadOp);
		attDesc.storeOp = static_cast<VkAttachmentStoreOp>(descr.isColor ? desc.colorStoreOp : desc.depthStoreOp);
		attDesc.finalLayout = static_cast<VkImageLayout>(descr.finalLayout);

		descs.emplace_back(attDesc);
	}
	return descs;
}

std::vector<VkAttachmentDescription> imp::RenderPassBase::CreateResolveAttachmentDescs(const RenderPassDesc& desc, const std::vector<SurfaceDesc>& surfaceDescs) const
{
	auto descs = std::vector<VkAttachmentDescription>();

	for (auto& descr : surfaceDescs)
	{
		VkAttachmentDescription attDesc = {};
		attDesc.flags = 1;
		attDesc.format = descr.format;
		attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attDesc.finalLayout = static_cast<VkImageLayout>(descr.finalLayout);

		descs.emplace_back(attDesc);
	}
	return descs;
}
