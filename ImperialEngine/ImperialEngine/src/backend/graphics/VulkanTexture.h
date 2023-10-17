#pragma once
#include "backend/VulkanResource.h"

namespace imp
{
	struct TextureCreationRequest;

	class VulkanTexture : public VulkanResource
	{
	public:
		VulkanTexture(const TextureCreationRequest& info, VkImageView view, VkSampler sampler);

	private:
		uint32_t m_ID;
		uint32_t m_ImageId;
		VkImageAspectFlags m_AspectMask;
		VkFilter m_MagFilter;
		VkFilter m_MinFilter;
		VkSamplerAddressMode m_AddressModeU;
		VkSamplerAddressMode m_AddressModeV;

		VkImageView m_View;
		VkSampler m_Sampler;
	};
}