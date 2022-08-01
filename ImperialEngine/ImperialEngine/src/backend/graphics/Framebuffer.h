#pragma once
#include "backend/VulkanResource.h"
#include <vector>

namespace imp
{
	class Surface;

	class Framebuffer : public VulkanResource
	{
	public:
		Framebuffer();
		Framebuffer(VkFramebuffer fb);

		bool StillValid(const std::vector<Surface>& surfaces) const;

		VkFramebuffer GetVkFramebuffer() const;

		void Destroy(VkDevice device) override;
	private:

		VkFramebuffer m_Framebuffer;
	};
}