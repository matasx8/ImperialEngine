#pragma once
#include <vulkan.h>

namespace imp
{
	class Framebuffer
	{
	public:
		Framebuffer();
	private:

		VkFramebuffer m_Framebuffer;
	};
}