#include "PipelineManager.h"

namespace imp
{
	PipelineManager::PipelineManager()
		: m_TemporarySinglePipeline()
	{
	}

	void PipelineManager::Initialize()
	{
	}

	void PipelineManager::CreatePipeline(const RenderPassBase& rp, const PipelineConfig& config)
	{
		VkPipelineShaderStageCreateInfo shaderStages[2];
		shaderStages[0] = MakeShaderStageCI(config.vertModule, VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = MakeShaderStageCI(config.fragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

		const auto vertInputState = MakeVertexInputStateCI();
		const auto inputAssembly = MakeInputAssemblyCI();


	}

	const Pipeline& PipelineManager::GetOrCreatePipeline(const RenderPassBase& rp, const PipelineConfig& config)
	{
		if (m_TemporarySinglePipeline.GetPipeline() == VK_NULL_HANDLE)
			CreatePipeline(rp, config);
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

	VkPipelineVertexInputStateCreateInfo PipelineManager::MakeVertexInputStateCI() const
	{
		const auto vertInputBindingDesc = MakeVertexBindingDesc();
		const auto vertInputAttrDesc = MakeVertexAttrDescs();

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

}