#include "VulkanShaderManager.h"
#include "backend/graphics/Graphics.h"
#include <optional>

namespace imp
{
	VulkanShaderManager::VulkanShaderManager()
		:
		m_ShaderMap(),
		m_DescriptorPool(),
		m_GlobalBuffers(),
		m_MaterialDataBuffers(),
		m_DrawDataBuffers(),
		m_DrawCommands(),
		m_BoundingVolumes(),
		m_DescriptorSets(),
		m_ComputeDescriptorSets(),
		m_DescriptorSetLayout(),
		m_ComputeDescriptorSetLayout()
	{
	}

	void VulkanShaderManager::Initialize(VkDevice device, VulkanMemory& memory, const EngineGraphicsSettings& settings, const MemoryProps& memProps, VulkanBuffer& drawCommands)
	{
		// TODO acceleration-part-1: print how much memory we're using
		static constexpr uint32_t kGlobalBufferSize = sizeof(GlobalData) * kGlobalBufferBindCount;
		static constexpr uint32_t kMaterialBufferSize = sizeof(ShaderDrawData) * kMaterialBufferBindCount;
		static constexpr uint32_t kDrawDataBufferSize = sizeof(ShaderDrawData) * kMaxDrawCount;
		static constexpr uint32_t kHostDrawCommandBufferSize = sizeof(IndirectDrawCmd) * (kMaxDrawCount + 31);
		static constexpr uint32_t kDrawCommandBufferSize = sizeof(VkDrawIndexedIndirectCommand) * (kMaxDrawCount + 31);
		static constexpr uint32_t kBoundingVolumeBufferSize = sizeof(SphereBV) * kMaxMeshCount;
		static constexpr auto kHostVisisbleCoherentFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		uint32_t hostMemUsed = 0;
		uint32_t deviceMemUsed = 0;

		// Here we create the needed buffers, descriptor sets and etc.
		// also the default material
		m_DescriptorPool = CreateDescriptorPool(device);

		// Here we're preparing for bindless rendering
		for (auto i = 0; i < settings.swapchainImageCount; i++)
		{
			// TODO: change to device local memory and use transfer queue to update data
			m_GlobalBuffers[i] = memory.GetBuffer(device, kGlobalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kHostVisisbleCoherentFlags, memProps);
			m_MaterialDataBuffers[i] = memory.GetBuffer(device, kMaterialBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kHostVisisbleCoherentFlags, memProps);
			m_DrawDataBuffers[i] = memory.GetBuffer(device, kDrawDataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kHostVisisbleCoherentFlags, memProps);

			// compute data:
			m_DrawCommands[i] = memory.GetBuffer(device, kHostDrawCommandBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, kHostVisisbleCoherentFlags, memProps);
			m_DrawCommands[i].MapWholeBuffer(device);

			hostMemUsed += kGlobalBufferSize + kMaterialBufferSize + kDrawDataBufferSize, kHostDrawCommandBufferSize;
		}

		// also compute data:
		m_BoundingVolumes = memory.GetBuffer(device, kBoundingVolumeBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProps);
		deviceMemUsed += kBoundingVolumeBufferSize;

		CreateMegaDescriptorSets(device);
		WriteUpdateDescriptorSets(device, m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_GlobalBuffers, sizeof(GlobalData), kGlobalBufferBindingSlot, kGlobalBufferBindCount, kEngineSwapchainExclusiveMax - 1);
		WriteUpdateDescriptorSets(device, m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_MaterialDataBuffers, sizeof(MaterialData), kMaterialBufferBindingSlot, kMaterialBufferBindingSlot, kEngineSwapchainExclusiveMax - 1);
		WriteUpdateDescriptorSets(device, m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_DrawDataBuffers, sizeof(ShaderDrawData), kDrawDataBufferBindingSlot, kMaxDrawCount, kEngineSwapchainExclusiveMax - 1);

		// Compute:
		// This array hack works but there's lots of redundant calculations. for single buffers
		// just calculate subbuffers once ant then update them to descriptorsets
		std::array<VulkanBuffer, 3> dc = { drawCommands, drawCommands, drawCommands };
		std::array<VulkanBuffer, 3> bv = { m_BoundingVolumes, m_BoundingVolumes, m_BoundingVolumes };
		WriteUpdateDescriptorSets(device, m_ComputeDescriptorSets.data(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_DrawCommands, kHostDrawCommandBufferSize, 0, 1, kEngineSwapchainExclusiveMax - 1);
		WriteUpdateDescriptorSets(device, m_ComputeDescriptorSets.data(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, dc, kDrawCommandBufferSize, 1, 1, kEngineSwapchainExclusiveMax - 1);
		WriteUpdateDescriptorSets(device, m_ComputeDescriptorSets.data(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bv, kBoundingVolumeBufferSize, 2, 1, kEngineSwapchainExclusiveMax - 1);

		CreateDefaultMaterial(device);

		printf("[Shader Memory] Successfully allocated %2.f MB of host domain memory and %.2f MB of device domain memory\n", hostMemUsed / 1024.0f / 1024.0f, deviceMemUsed / 1024.0f / 1024.0f);
	}

	VulkanShader VulkanShaderManager::GetShader(const std::string& shaderName) const
	{
		const auto shader = m_ShaderMap.find(shaderName);
		if (shader != m_ShaderMap.end())
			return shader->second;
		return VulkanShader();	// TODO: return default/error shader
	}

	VkDescriptorSet VulkanShaderManager::GetDescriptorSet(uint32_t idx) const
	{
		return m_DescriptorSets[idx];
	}

	VkDescriptorSet VulkanShaderManager::GetComputeDescriptorSet(uint32_t idx) const
	{
		return m_ComputeDescriptorSets[idx];
	}

	VkDescriptorSetLayout VulkanShaderManager::GetDescriptorSetLayout() const
	{
		return m_DescriptorSetLayout;
	}

	VkDescriptorSetLayout VulkanShaderManager::GetComputeDescriptorSetLayout() const
	{
		return m_ComputeDescriptorSetLayout;
	}

	void VulkanShaderManager::CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req)
	{
		const VulkanShader vertexShader(CreateShaderModule(device, *req.vertexSpv.get()));
		const VulkanShader vertexIndShader(CreateShaderModule(device, *req.vertexIndSpv.get()));
		const VulkanShader fragmentShader(CreateShaderModule(device, *req.fragmentSpv.get()));
		m_ShaderMap[req.shaderName + ".vert"] = vertexShader;
		m_ShaderMap[req.shaderName + ".ind.vert"] = vertexIndShader;
		m_ShaderMap[req.shaderName + ".frag"] = fragmentShader;
	}

	void VulkanShaderManager::CreateComputePrograms(VkDevice device, PipelineManager& pipeManager, const CmdRsc::ComputeProgramCreationRequest& req)
	{
		const auto shader = VulkanShader(CreateShaderModule(device, *req.spv.get()));
		m_ShaderMap[req.shaderName + ".comp"] = shader;

		ComputePipelineConfig config = { shader.GetShaderModule(), m_DescriptorSetLayout, m_ComputeDescriptorSetLayout };
		pipeManager.CreateComputePipeline(device, config);
	}

	void VulkanShaderManager::UpdateGlobalData(VkDevice device, uint32_t descriptorSetIdx, const GlobalData& data)
	{
		UpdateDescriptorData(device, m_GlobalBuffers[descriptorSetIdx], sizeof(GlobalData), 0, &data);
	}

	void VulkanShaderManager::UpdateDrawData(VkDevice device, uint32_t descriptorSetIdx, const std::vector<DrawDataSingle> drawData)
	{
		// We're relying that the descriptors are registered
		std::vector<ShaderDrawData> shaderData;
		for (const auto& draw : drawData)
		{
			ShaderDrawData dat;
			dat.transform = draw.Transform;
			dat.materialIndex = kDefaultMaterialIndex;
			dat.isEnabled = true;

			shaderData.push_back(dat);
		}
		UpdateDescriptorData(device, m_DrawDataBuffers[descriptorSetIdx], drawData.size() * sizeof(ShaderDrawData), 0, shaderData.data());
	}

	VkDescriptorPool VulkanShaderManager::CreateDescriptorPool(VkDevice device)
	{
		static constexpr uint32_t kDescriptorCount = 128;
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

	VkShaderModule VulkanShaderManager::CreateShaderModule(VkDevice device, const std::string& shaderCode)
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

	static void AllocateDescriptorSets(VkDevice device, VkDescriptorSet* dst, VkDescriptorPool pool, uint32_t descriptorSetCount, VkDescriptorSetLayout* layouts, const void* pnext)
	{
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = pool;
		allocateInfo.descriptorSetCount = descriptorSetCount;
		allocateInfo.pSetLayouts = layouts;
		allocateInfo.pNext = pnext;

		auto res = vkAllocateDescriptorSets(device, &allocateInfo, dst);
		assert(res == VK_SUCCESS);
	}

	void VulkanShaderManager::CreateMegaDescriptorSets(VkDevice device)
	{
		CreateMegaDescriptorSetLayout(device);		// set 0
		CreateComputeDescriptorSetLayout(device);	// set 1

		// variable descriptor counts
		VkDescriptorSetVariableDescriptorCountAllocateInfo variable_info = {};
		variable_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variable_info.descriptorSetCount = m_DescriptorSets.size();
		std::array<uint32_t, kBindingCount> descriptorCounts = { kMaxDrawCount, kMaxDrawCount, kMaxDrawCount };
		variable_info.pDescriptorCounts = descriptorCounts.data();

		std::array<VkDescriptorSetLayout, kEngineSwapchainExclusiveMax - 1> layouts = { m_DescriptorSetLayout, m_DescriptorSetLayout, m_DescriptorSetLayout };
		AllocateDescriptorSets(device, m_DescriptorSets.data(), m_DescriptorPool, m_DescriptorSets.size(), layouts.data(), &variable_info);

		std::array<VkDescriptorSetLayout, kEngineSwapchainExclusiveMax - 1> computeLayouts = { m_ComputeDescriptorSetLayout, m_ComputeDescriptorSetLayout, m_ComputeDescriptorSetLayout };
		AllocateDescriptorSets(device, m_ComputeDescriptorSets.data(), m_DescriptorPool, m_ComputeDescriptorSets.size(), computeLayouts.data(), nullptr);
	}

	void VulkanShaderManager::CreateMegaDescriptorSetLayout(VkDevice device)
	{
		const auto globalBufferBinding = CreateDescriptorBinding(kGlobalBufferBindingSlot, kGlobalBufferBindCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
		const auto materialDataBufferBinding = CreateDescriptorBinding(kMaterialBufferBindingSlot, kMaterialBufferBindCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);
		const auto drawDataBufferBinding = CreateDescriptorBinding(kDrawDataBufferBindingSlot, kMaxDrawCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);

		std::array<VkDescriptorSetLayoutBinding, kBindingCount> bindings = { globalBufferBinding, materialDataBufferBinding, drawDataBufferBinding };

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

	void VulkanShaderManager::CreateComputeDescriptorSetLayout(VkDevice device)
	{
		const auto drawCommandStagingBufferBinding = CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		const auto drawCommandBufferBinding = CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		const auto boundingVolumeBinding = CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

		std::array<VkDescriptorSetLayoutBinding, kComputeBindingCount> bindings = { drawCommandStagingBufferBinding, drawCommandBufferBinding, boundingVolumeBinding };

		static constexpr VkDescriptorBindingFlags nonVariableBindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

		std::array<VkDescriptorBindingFlags, kComputeBindingCount> multipleFlags = { nonVariableBindingFlags, nonVariableBindingFlags, nonVariableBindingFlags };

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
		bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlags.bindingCount = kComputeBindingCount;
		bindingFlags.pBindingFlags = multipleFlags.data();

		m_ComputeDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayoutCreateInfo dci;
		dci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dci.pNext = &bindingFlags;
		dci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		dci.bindingCount = kComputeBindingCount;
		dci.pBindings = bindings.data();
		const auto res = vkCreateDescriptorSetLayout(device, &dci, nullptr, &m_ComputeDescriptorSetLayout);
		assert(res == VK_SUCCESS);
	}

	VkDescriptorSetLayoutBinding VulkanShaderManager::CreateDescriptorBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkShaderStageFlags stageFlags)
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

	void VulkanShaderManager::WriteUpdateDescriptorSets(VkDevice device, VkDescriptorSet* dSets, VkDescriptorType type, auto& buffers, size_t descriptorDataSize, uint32_t bindSlot, uint32_t descriptorCount, uint32_t dSetCount)
	{
		// I'm almost certain we should just vkUpdateDescriptorSets upfront for the whole buffer
		for (auto i = 0; i < dSetCount; i++)
		{
			std::vector<VkDescriptorBufferInfo> bis;
			for (auto j = 0; j < descriptorCount; j++)
				bis.push_back(buffers[i].RegisterSubBuffer(descriptorDataSize));

			const uint32_t index = bis.front().offset / descriptorDataSize;

			// so far we support only same data size
			assert(bis.front().offset % descriptorDataSize == 0);

			VkWriteDescriptorSet drawWrite = {};
			drawWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			drawWrite.dstSet = dSets[i];
			drawWrite.dstBinding = bindSlot;
			drawWrite.dstArrayElement = index;
			drawWrite.descriptorType = type;
			drawWrite.descriptorCount = descriptorCount;
			drawWrite.pBufferInfo = bis.data();

			vkUpdateDescriptorSets(device, 1, &drawWrite, 0, nullptr);
		}
	}

	void VulkanShaderManager::UpdateDescriptorData(VkDevice device, VulkanBuffer& buffer, size_t size, uint32_t offset, const void* data)
	{
		assert(data);
		void* dataMap;
		const auto res = vkMapMemory(device, buffer.GetMemory(), offset, size, 0, &dataMap);
		memcpy(dataMap, data, size);
		vkUnmapMemory(device, buffer.GetMemory());
		assert(res == VK_SUCCESS);
	}

	void VulkanShaderManager::CreateDefaultMaterial(VkDevice device)
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

		for (auto i = 0; i < kEngineSwapchainExclusiveMax - 1; i++)
		{
			// example data for material
			const MaterialData data = { glm::vec4(0.75f, 0.99f, 0.99f, 1.0f) };
			UpdateDescriptorData(device, m_MaterialDataBuffers[i], sizeof(MaterialData), 0, &data);
		}
	}
}