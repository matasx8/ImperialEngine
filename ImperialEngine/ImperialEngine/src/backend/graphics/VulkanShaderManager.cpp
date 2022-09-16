#include "VulkanShaderManager.h"
#include "backend/graphics/Graphics.h"
#include <optional>

imp::VulkanShaderManager::VulkanShaderManager()
	: m_ShaderMap()
{
}

void imp::VulkanShaderManager::Initialize(VkDevice device, VulkanMemory& memory, const EngineGraphicsSettings& settings, const MemoryProps& memProps)
{
	static constexpr uint32_t kBufferSize = 1024 * 1024;
	// Here we create the needed buffers, descriptor sets and etc.
	// also the default material
	m_DescriptorPool = CreateDescriptorPool(device);

	for (auto i = 0; i < settings.swapchainImageCount; i++)
	{
		// TODO: change to device local memory and use transfer queue to update data
		m_GlobalBuffers[i]			= memory.GetBuffer(device, kBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memProps);
		m_MaterialDataBuffers[i]	= memory.GetBuffer(device, kBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memProps);
		m_DrawDataBuffers[i]		= memory.GetBuffer(device, kBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memProps);
	}

	CreateMegaDescriptorSets(device);
	CreateDefaultMaterial(device);
}

imp::VulkanShader imp::VulkanShaderManager::GetShader(const std::string& shaderName)
{
	const auto shader = m_ShaderMap.find(shaderName);
	if (shader != m_ShaderMap.end())
		return shader->second;
	return VulkanShader();	// TODO: return default/error shader
}

VkDescriptorSet imp::VulkanShaderManager::GetDescriptorSet(uint32_t idx)
{
	return m_DescriptorSets[idx];
}

void imp::VulkanShaderManager::CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req)
{
	const auto vertexShaderModule = CreateShaderModule(device, *req.vertexSpv.get());
	const auto fragmentShaderModule = CreateShaderModule(device, *req.fragmentSpv.get());

	const VulkanShader vertexShader(vertexShaderModule);
	const VulkanShader fragmentShader(fragmentShaderModule);
	m_ShaderMap[req.shaderName + ".vert"] = vertexShader;
	m_ShaderMap[req.shaderName + ".frag"] = fragmentShader;
}

void imp::VulkanShaderManager::UpdateGlobalData(VkDevice device, uint32_t descriptorSetIdx, const GlobalData& data)
{
	UpdateDescriptorData(device, m_GlobalBuffers[descriptorSetIdx], sizeof(GlobalData), 0, &data);
}

void imp::VulkanShaderManager::UpdateDrawData(VkDevice device, uint32_t descriptorSetIdx, const std::vector<DrawDataSingle> drawData)
{
	// We're relying that the descriptors are registered
	UpdateDescriptorData(device, m_DrawDataBuffers[descriptorSetIdx], drawData.size() * sizeof(DrawDataSingle), 0, drawData.data());
}

void imp::VulkanShaderManager::RegisterDraws(VkDevice device, uint32_t numDraws)
{
	// currently no way to add more
	WriteUpdateDescriptorSets(device, m_DrawDataBuffers, sizeof(ShaderDrawData), kDrawDataBufferBindingSlot);
}

VkDescriptorPool imp::VulkanShaderManager::CreateDescriptorPool(VkDevice device)
{
	// I wonder if this is what's causing high memory usage?
	// Can we go over descriptor count even if we have variable descriptor count?
	// probably no, since the main point of that is we dont have o specify hardcoded number in shader
	static constexpr uint32_t kDescriptorCount = 128; // so this means we can have max 128 draws..
	static constexpr uint32_t kDescriptorPoolSizeCount = 6;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::array< VkDescriptorPoolSize, kDescriptorPoolSizeCount> poolSizes;
	poolSizes[0] = { VK_DESCRIPTOR_TYPE_SAMPLER, kDescriptorCount };
	poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kDescriptorCount };
	poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kDescriptorCount };
	poolSizes[3] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kDescriptorCount };
	poolSizes[4] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kDescriptorCount };
	poolSizes[5] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kDescriptorCount };

	VkDescriptorPoolCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.pNext = nullptr;
	ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	ci.maxSets = kDescriptorCount;
	ci.poolSizeCount = kDescriptorPoolSizeCount;
	ci.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool);
	return descriptorPool;
}

VkShaderModule imp::VulkanShaderManager::CreateShaderModule(VkDevice device, const std::string& shaderCode)
{
	VkShaderModuleCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule shaderModule;
	auto res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	assert(res == VK_SUCCESS);
	return shaderModule;
}

void imp::VulkanShaderManager::CreateMegaDescriptorSets(VkDevice device)
{
	CreateMegaDescriptorSetLayout(device);

	VkDescriptorSetVariableDescriptorCountAllocateInfo variable_info = {};
	variable_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variable_info.descriptorSetCount = m_DescriptorSets.size();
	// these descriptor counts must match binding slots
	std::array<uint32_t, kBindingCount> descriptorCounts = { kGlobalBufferBindCount, kMaterialBufferBindCount, kMaxDrawCount };
	// for example the global buffer is binding #1 so it needs two descriptors
	// so only the last binding can have the truly variable count
	variable_info.pDescriptorCounts = descriptorCounts.data();

	std::array<VkDescriptorSetLayout, kEngineSwapchainExclusiveMax - 1> layouts = { m_DescriptorSetLayout, m_DescriptorSetLayout, m_DescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = m_DescriptorPool;
	allocateInfo.descriptorSetCount = m_DescriptorSets.size();
	allocateInfo.pSetLayouts = layouts.data();
	allocateInfo.pNext = &variable_info;

	auto res = vkAllocateDescriptorSets(device, &allocateInfo, m_DescriptorSets.data());
	assert(res == VK_SUCCESS);
}

void imp::VulkanShaderManager::CreateMegaDescriptorSetLayout(VkDevice device)
{
	const auto globalBufferBinding		    = CreateDescriptorBinding(kGlobalBufferBindingSlot, kGlobalBufferBindCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
	const auto materialDataBufferBinding	= CreateDescriptorBinding(kMaterialBufferBindingSlot, kMaterialBufferBindCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);
	const auto drawDataBufferBinding		= CreateDescriptorBinding(kDrawDataBufferBindingSlot, kMaxDrawCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);

	std::array<VkDescriptorSetLayoutBinding, kBindingCount> bindings = { globalBufferBinding, materialDataBufferBinding, drawDataBufferBinding};

	const VkDescriptorBindingFlags nonVariableBindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
	const VkDescriptorBindingFlags variableBindingFlags = nonVariableBindingFlags | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

	std::array< VkDescriptorBindingFlags, kBindingCount> multipleFlags = { nonVariableBindingFlags, nonVariableBindingFlags, variableBindingFlags };

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = kBindingCount;
	bindingFlags.pBindingFlags = multipleFlags.data();

	m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayoutCreateInfo dci;
	dci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dci.pNext = &bindingFlags;
	dci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	dci.bindingCount = kBindingCount;
	dci.pBindings = bindings.data();
	const auto res = vkCreateDescriptorSetLayout(device, &dci, nullptr, &m_DescriptorSetLayout);
	assert(res == VK_SUCCESS);
}

VkDescriptorSetLayoutBinding imp::VulkanShaderManager::CreateDescriptorBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	assert(descriptorCount);
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
	descriptorSetLayoutBinding.binding = binding;
	descriptorSetLayoutBinding.descriptorCount = descriptorCount;
	descriptorSetLayoutBinding.descriptorType = type;
	descriptorSetLayoutBinding.stageFlags = stageFlags; // is this good? global stuf should probably be accessed evberywhere
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	return descriptorSetLayoutBinding;
}

uint32_t imp::VulkanShaderManager::WriteUpdateDescriptorSets(VkDevice device, auto& buffers, size_t size, uint32_t bindSlot)
{
	const auto index = buffers[0].FindNewSubBufferIndex(size);
	for (auto i = 0; i < kEngineSwapchainExclusiveMax - 1; i++)
	{
		const auto bufferInfo = buffers[i].RegisterSubBuffer(size);

		VkWriteDescriptorSet drawWrite = {};
		drawWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		drawWrite.dstSet = m_DescriptorSets[i];
		drawWrite.dstBinding = bindSlot; // should this be slot or the new slot?
		drawWrite.dstArrayElement = 0;
		drawWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		drawWrite.descriptorCount = 1;
		drawWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &drawWrite, 0, nullptr);
	}
	return index;
}

void imp::VulkanShaderManager::UpdateDescriptorData(VkDevice device, VulkanBuffer& buffer, size_t size, uint32_t offset, const void* data)
{
	assert(data);
	void* dataMap;
	const auto res = vkMapMemory(device, buffer.GetMemory(), offset, size, 0, &dataMap);
	memcpy(dataMap, data, size);
	vkUnmapMemory(device, buffer.GetMemory());
	assert(res == VK_SUCCESS);
}

void imp::VulkanShaderManager::CreateDefaultMaterial(VkDevice device)
{
	// Create material should probably only ask to add stuff to material data and not global data.
	//
	// What can go wrong?
	// 1. Buffer can run out of memory
	// 1.1 VulkanBuffer needs implementation for managing memory
	// 1.2 Should be able to track what part of the memory is used by a descriptor
	// 1.3 What happen when memory is fragmented or we need more?
	// 2. DescriptorPool can run out of descriptors
	// -- For now simple enough to add and add more stuff, remove and leave hole?

	// this is probably CretaeMaterial()
	const auto idx = WriteUpdateDescriptorSets(device, m_MaterialDataBuffers, sizeof(MaterialData), kMaterialBufferBindingSlot); // register
	assert(idx == kDefaultMaterialIndex);	// should get material data index 0
	for (auto i = 0; i < kEngineSwapchainExclusiveMax - 1; i++)
	{
		const MaterialData data = { glm::vec4(0.75f, 0.99f, 0.99f, 1.0f) };
		UpdateDescriptorData(device, m_MaterialDataBuffers[i], sizeof(MaterialData), 0, &data);
	}
}
