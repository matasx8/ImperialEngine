#include "backend/VariousTypeDefinitions.h"
#include "PipelineManager.h"
#include <GLM/mat4x4.hpp>

namespace imp
{
	// TODO compute-drawindirect: rework pipeline manager
	PipelineManager::PipelineManager()
		: m_PipelineMap(), m_ComputePipelineMap()
	{
	}

	Pipeline PipelineManager::CreatePipeline(VkDevice device, const RenderPass& rp, const PipelineConfig& config)
	{
		// TODO: have a 'base' pipeline that I can use to create pipeline derivatives?

		const bool meshPipeline = config.meshModule != VK_NULL_HANDLE;
		const auto shaderStageCount = meshPipeline ? 3 : 2;

		VkPipelineShaderStageCreateInfo shaderStages[3];
		shaderStages[0] = MakeShaderStageCI(meshPipeline ? config.meshModule : config.vertModule, meshPipeline ? VK_SHADER_STAGE_MESH_BIT_EXT : VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = MakeShaderStageCI(config.fragModule, VK_SHADER_STAGE_FRAGMENT_BIT);
		if (meshPipeline)
			shaderStages[2] = MakeShaderStageCI(config.taskModule, VK_SHADER_STAGE_TASK_BIT_EXT);

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
		pushRange.size = sizeof(uint32_t); // index into draw data
		pushRange.offset = 0;
		pushRange.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_VERTEX_BIT;
		std::vector<VkDescriptorSetLayout> layouts = { config.descriptorSetLayout };
		if (meshPipeline) layouts.push_back(config.descriptorSetLayout2);

		const auto pipelineLayoutCI = MakePipelineLayoutCI(&pushRange, layouts);
		const auto pipelineLayout = MakePipelineLayout(device, pipelineLayoutCI);

		const auto pipelineCI = MakePipelineCI(shaderStages, shaderStageCount, &vertInputState, &inputAssembly, &viewportState, nullptr, &rasterizationState, &msaaState, &colorBlendState, &depthStencilState, pipelineLayout, rp.GetVkRenderPass(), 0);
		const auto pipeline = MakePipeline(device, pipelineCI);

		return Pipeline(pipeline, pipelineLayout);
	}

	const Pipeline& PipelineManager::GetOrCreatePipeline(VkDevice device, const RenderPass& rp, const PipelineConfig& config)
	{
		if (!m_PipelineMap.contains(config))
		{
			const auto newPipe = CreatePipeline(device, rp, config);
			m_PipelineMap[config] = newPipe;
		}
		return m_PipelineMap.at(config);
	}

	const Pipeline& PipelineManager::GetComputePipeline(const ComputePipelineConfig& config)
	{
		return m_ComputePipelineMap.at(config);
	}

	void PipelineManager::CreateComputePipeline(VkDevice device, const ComputePipelineConfig& config)
	{
		VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

		VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage.module = config.computeModule;
		stage.pName = "main";

		std::vector<VkDescriptorSetLayout> dsetLayouts = { config.descriptorSetLayout, config.descriptorSetLayout2 };

		// TODO mesh: is this needed anymore?
		VkPushConstantRange pushRange;
		pushRange.size = sizeof(glm::vec4) * 6 + sizeof(uint32_t);
		pushRange.offset = 0;
		pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		const auto pipeLayoutCI = MakePipelineLayoutCI(&pushRange, dsetLayouts);
		const auto pipeLayout = MakePipelineLayout(device, pipeLayoutCI);

		createInfo.stage = stage;
		createInfo.layout = pipeLayout;

		VkPipeline pipeline = VK_NULL_HANDLE;
		const auto res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, 0, &pipeline);
		assert(res == VK_SUCCESS);

		const auto pipe = Pipeline(pipeline, pipeLayout);
		m_ComputePipelineMap[config] = pipe;
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
		constexpr int inputAttributes = 0;
		std::vector<VkVertexInputAttributeDescription> descs(inputAttributes);
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
		ci.vertexBindingDescriptionCount = 0;
		ci.pVertexBindingDescriptions = nullptr;
		ci.vertexAttributeDescriptionCount = 0;// static_cast<uint32_t>(vertInputAttrDesc.size());
		ci.pVertexAttributeDescriptions = nullptr;// vertInputAttrDesc.data();
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
		ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
		return ci;
	}

	VkPipelineMultisampleStateCreateInfo PipelineManager::MakeMSAAStateCI(const RenderPass& rp) const
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
		ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		return ci;
	}

	VkPipelineLayoutCreateInfo PipelineManager::MakePipelineLayoutCI(const VkPushConstantRange* pushRange, const std::vector<VkDescriptorSetLayout>& layouts) const
	{
		VkPipelineLayoutCreateInfo ci;
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		ci.setLayoutCount = layouts.size();
		ci.pSetLayouts = layouts.data();
		ci.pushConstantRangeCount = pushRange ? 1 : 0;
		ci.pPushConstantRanges = pushRange;
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

	VkGraphicsPipelineCreateInfo PipelineManager::MakePipelineCI(const VkPipelineShaderStageCreateInfo* shaderStages, uint32_t stageCount, const VkPipelineVertexInputStateCreateInfo* vertexInputCreateInfo, const
		VkPipelineInputAssemblyStateCreateInfo* inputAssembly, const VkPipelineViewportStateCreateInfo* viewportStateCreateInfo, const
		VkPipelineDynamicStateCreateInfo* dynamicState, const VkPipelineRasterizationStateCreateInfo* rasterizerCreateInfo, const
		VkPipelineMultisampleStateCreateInfo* multisamplingCreateInfo, const VkPipelineColorBlendStateCreateInfo* colourBlendingCreateInfo, const
		VkPipelineDepthStencilStateCreateInfo* depthStencilCreateInfo, const VkPipelineLayout pipelineLayout, const
		VkRenderPass renderPass, VkPipelineCreateFlags flags) const
	{
		VkGraphicsPipelineCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		ci.stageCount = stageCount;
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