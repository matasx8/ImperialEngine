#pragma once
#include <vulkan.h>

namespace imp
{
	class Framebuffer
	{
	public:
		Framebuffer();
		Framebuffer(VkFramebuffer fb);

		VkFramebuffer GetVkFramebuffer() const;

		void Destroy(VkDevice device);
	private:

		VkFramebuffer m_Framebuffer;
	};
}