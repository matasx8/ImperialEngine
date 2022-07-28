#include "Framebuffer.h"

imp::Framebuffer::Framebuffer()
	: m_Framebuffer()
{
}

imp::Framebuffer::Framebuffer(VkFramebuffer fb)
	: m_Framebuffer(fb)
{
}

bool imp::Framebuffer::StillValid(const std::vector<Surface>& surfaces) const
{
	// TODO: this
	return false; // for now lets create new fb each time
}

VkFramebuffer imp::Framebuffer::GetVkFramebuffer() const
{
	return m_Framebuffer;
}

void imp::Framebuffer::Destroy(VkDevice device)
{

}
