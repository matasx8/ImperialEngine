#include "VulkanTexture.h"
#include "backend/VariousTypeDefinitions.h"

namespace imp
{
	imp::VulkanTexture::VulkanTexture(const TextureCreationRequest& info, VkImageView view, VkSampler sampler)
	: m_ID(info.id)
	, m_ImageId(info.imageId)
	, m_AspectMask(info.imageAspect)
	, m_MagFilter(info.magFilter)
	, m_MinFilter(info.minFilter)
	, m_AddressModeU(info.addressModeU)
	, m_AddressModeV(info.addressModeV)
	, m_View(view)
	, m_Sampler(sampler)
	{
	}

	uint32_t VulkanTexture::GetImageId() const
	{
		return m_ImageId;
	}

	VkImageView VulkanTexture::GetImageView() const
	{
		return m_View;
	}

	VkSampler VulkanTexture::GetSampler() const
	{
		return m_Sampler;
	}
}