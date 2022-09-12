#include "PipelineManager.h"
#include <GLM/mat4x4.hpp>

namespace imp
{
	PipelineManager::PipelineManager()
		: m_TemporarySinglePipeline()
	{
	}

	void PipelineManager::Initialize()
	{
	}

	void PipelineManager::CreatePipeline(VkDevice device, const RenderPassBase& rp, const PipelineConfig& config)
	{
		// TODO: have a 'base' pipeline that I can use to create pipeline derivatives?
		VkPipelineShaderStageCreateInfo shaderStages[2];
		shaderStages[0] = MakeShaderStageCI(config.vertModule, VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = MakeShaderStageCI(config.fragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

		const auto vertInputBindingDesc = MakeVertexBindingDesc();
		const auto vertInputAttrDesc = MakeVertexAttrDescs();
		const auto vertInputState = MakeVertexInputStateCI(vertInputBindingDesc, vertInputAttrDesc);
		const auto inputAssembly = MakeInputAssemblyCI();

		const auto viewport = rp.GetViewport();
		const auto scissor = rp.GetScissor();
		const auto viewportState = MakeViewportStateCI(viewport, scissor);
		const auto rasterizationState = MakeRasterizationSateCI();
		const auto msaaState = MakeMSAAStateCI(rp);

		VkPipelineColorBlendAttachmentState state;
		state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		state.blendEnable = VK_FALSE;
		state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		state.colorBlendOp = VK_BLEND_OP_ADD;
		state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		state.alphaBlendOp = VK_BLEND_OP_ADD;
		const auto colorBlendState = MakeColorBlendStateCI(state);
		const auto depthStencilState = MakeDepthStencilStateCI();

		VkPushConstantRange pushRange;
		pushRange.size = sizeof(glm::mat4x4) * 2;	// temporary so I can push ViewProjection matrix without having descriptors
		pushRange.offset = 0;
		pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		const auto pipelineLayoutCI = MakePipelineLayoutCI(pushRange);
		const auto pipelineLayout = MakePipelineLayout(device, pipelineLayoutCI);

		const auto pipelineCI = MakePipelineCI(shaderStages, &vertInputState, &inputAssembly, &viewportState, nullptr, &rasterizationState, &msaaState, &colorBlendState, &depthStencilState, pipelineLayout, rp.GetVkRenderPass(), 0);
		const auto pipeline = MakePipeline(device, pipelineCI);

		m_TemporarySinglePipeline = Pipeline(pipeline, pipelineLayout);
	}

	const Pipeline& PipelineManager::GetOrCreatePipeline(VkDevice device, const RenderPassBase& rp, const PipelineConfig& config)
	{
		if (m_TemporarySinglePipeline.GetPipeline() == VK_NULL_HANDLE)
			CreatePipeline(device, rp, config);
		return m_TemporarySinglePipeline;
	}

	auto PipelineManager::GetPipeline() const
	{	// trying out modern c++ magic
		return m_TemporarySinglePipeline.GetPipeline() == VK_NULL_HANDLE ? std::make_optional<std::reference_wrapper<const Pipeline>>(std::reference_wrapper<const Pipeline>(m_TemporarySinglePipeline)) : std::nullopt;
	}

	void PipelineManager::Destroy(VkDevice device)
	{
	}
	VkPipelineShaderStageCreateInfo PipelineManager::MakeShaderStageCI(VkShaderModule module, VkShaderStageFlagBits stage) const
	{
		VkPipelineShaderStageCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ci.stage = stage;
		ci.module = module;
		ci.pName = "main";
		ci.pNext = nullptr;
		ci.flags = 0;
		ci.pSpecializationInfo = nullptr;
		return ci;
	}
	VkVertexInputBindingDescription PipelineManager::MakeVertexBindingDesc() const
	{
		VkVertexInputBindingDescription desc;
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // get from pipe config
		return desc;
	}
	std::vector<VkVertexInputAttributeDescription> PipelineManager::MakeVertexAttrDescs() const
	{
		constexpr int inputAttributes = 3;
		std::vector<VkVertexInputAttributeDescription> descs(inputAttributes);
		descs[0] = MakeVertexAttrDesc(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
		descs[1] = MakeVertexAttrDesc(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm));
		descs[2] = MakeVertexAttrDesc(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex));
		return descs;
	}

	VkVertexInputAttributeDescription PipelineManager::MakeVertexAttrDesc(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) const
	{
		VkVertexInputAttributeDescription desc;
		desc.binding = binding;
		desc.location = location;
		desc.format = format;
		desc.offset = offset;
		return desc;
	}

	VkPipelineVertexInputStateCreateInfo PipelineManager::MakeVertexInputStateCI(const auto& vertInputBindingDesc, const auto& vertInputAttrDesc) const
	{
		VkPipelineVertexInputStateCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		ci.vertexBindingDescriptionCount = 1;
		ci.pVertexBindingDescriptions = &vertInputBindingDesc;
		ci.vertexAttributeDescriptionCount = vertInputAttrDesc.size();
		ci.pVertexAttributeDescriptions = vertInputAttrDesc.data();
		ci.pNext = nullptr;
		ci.flags = 0;
		return ci;
	}

	VkPipelineInputAssemblyStateCreateInfo PipelineManager::MakeInputAssemblyCI() const
	{
		VkPipelineInputAssemblyStateCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		ci.primitiveRestartEnable = VK_FALSE;
		ci.pNext = nullptr;
		ci.flags = 0;
		return ci;
	}

	VkPipelineViewportStateCreateInfo PipelineManager::MakeViewportStateCI(const auto& viewport, const auto& scissor) const
	{
		VkPipelineViewportStateCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ci.viewportCount = 1;
		ci.pViewports = &viewport;
		ci.scissorCount = 1;
		ci.pScissors = &scissor;
		ci.pNext = nullptr;
		ci.flags = 0;

		return ci;
	}

	VkPipelineRasterizationStateCreateInfo PipelineManager::MakeRasterizationSateCI() const
	{
		VkPipelineRasterizationStateCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		ci.polygonMode = VK_POLYGON_MODE_FILL;
		ci.lineWidth = 1.0f;
		ci.cullMode = VK_CULL_MODE_BACK_BIT;
		ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		return ci;
	}

	VkPipelineMultisampleStateCreateInfo PipelineManager::MakeMSAAStateCI(const RenderPassBase& rp) const
	{
		VkPipelineMultisampleStateCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ci.rasterizationSamples = static_cast<VkSampleCountFlagBits>(rp.GetRenderPassDesc().colorSurfaces[0].msaaCount);
		return ci;
	}

	VkPipelineColorBlendStateCreateInfo PipelineManager::MakeColorBlendStateCI(const auto& blendAttState) const
	{
		VkPipelineColorBlendStateCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ci.attachmentCount = 1;
		ci.pAttachments = &blendAttState;
		return ci;
	}

	VkPipelineDepthStencilStateCreateInfo PipelineManager::MakeDepthStencilStateCI() const
	{
		VkPipelineDepthStencilStateCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		ci.depthTestEnable = VK_TRUE;
		ci.depthWriteEnable = VK_TRUE;
		ci.depthCompareOp = VK_COMPARE_OP_LESS;
		return ci;
	}

	VkPipelineLayoutCreateInfo PipelineManager::MakePipelineLayoutCI(const auto& pushRange) const
	{
		assert(pushRange.size);
		VkPipelineLayoutCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		ci.setLayoutCount = 0;
		ci.pSetLayouts = nullptr;
		ci.pushConstantRangeCount = 1;
		ci.pPushConstantRanges = &pushRange;
		ci.pNext = nullptr;
		ci.flags = 0;
		return ci;
	}

	VkPipelineLayout PipelineManager::MakePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& ci) const
	{
		VkPipelineLayout layout;
		const auto res = vkCreatePipelineLayout(device, &ci, nullptr, &layout);
		assert(res == VK_SUCCESS);
		return layout;
	}

	VkGraphicsPipelineCreateInfo PipelineManager::MakePipelineCI(const VkPipelineShaderStageCreateInfo* shaderStages, const VkPipelineVertexInputStateCreateInfo* vertexInputCreateInfo, const
		VkPipelineInputAssemblyStateCreateInfo* inputAssembly, const VkPipelineViewportStateCreateInfo* viewportStateCreateInfo, const
		VkPipelineDynamicStateCreateInfo* dynamicState, const VkPipelineRasterizationStateCreateInfo* rasterizerCreateInfo, const
		VkPipelineMultisampleStateCreateInfo* multisamplingCreateInfo, const VkPipelineColorBlendStateCreateInfo* colourBlendingCreateInfo, const
		VkPipelineDepthStencilStateCreateInfo* depthStencilCreateInfo, const VkPipelineLayout pipelineLayout, const
		VkRenderPass renderPass, VkPipelineCreateFlags flags) const
	{
		const auto stage = shaderStages[1];
		VkGraphicsPipelineCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		ci.stageCount = 2;
		ci.pStages = shaderStages;
		ci.pVertexInputState = vertexInputCreateInfo;
		ci.pInputAssemblyState = inputAssembly;
		ci.pViewportState = viewportStateCreateInfo;
		ci.pDynamicState = dynamicState;
		ci.pRasterizationState = rasterizerCreateInfo;
		ci.pMultisampleState = multisamplingCreateInfo;
		ci.pColorBlendState = colourBlendingCreateInfo;
		ci.pDepthStencilState = depthStencilCreateInfo;
		ci.layout = pipelineLayout;
		ci.renderPass = renderPass;
		ci.subpass = 0;
		ci.flags = flags;
		ci.pNext = nullptr;

		return ci;
	}

	VkPipeline PipelineManager::MakePipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& ci) const
	{
		VkPipeline pipeline;
		const auto res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);
		return pipeline;
	}

}