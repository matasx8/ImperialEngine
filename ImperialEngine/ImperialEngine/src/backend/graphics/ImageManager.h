#pragma once
#include "Utils/NonCopyable.h"
#include "backend/graphics/VulkanImage.h"

namespace imp
{
	class VulkanMemory;
	struct ImageCreationRequest;
	struct MemoryProps;

	class ImageManager : NonCopyable
	{
	public:
		ImageManager(VulkanMemory& memory);

		VulkanImage GetImage(VkDevice device, const ImageCreationRequest& req, const MemoryProps& memProps);

	private:

		VulkanMemory& m_Memory;
	};
}