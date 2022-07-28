#pragma once
#include <vulkan.h>
#include <vector>

namespace imp
{
	class Surface;

	class Framebuffer
	{
	public:
		Framebuffer();
		Framebuffer(VkFramebuffer fb);

		bool StillValid(const std::vector<Surface>& surfaces) const;

		VkFramebuffer GetVkFramebuffer() const;

		void Destroy(VkDevice device);
	private:

		VkFramebuffer m_Framebuffer;
	};
}