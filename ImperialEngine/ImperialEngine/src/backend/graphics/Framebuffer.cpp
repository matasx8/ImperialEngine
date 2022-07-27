#include "Framebuffer.h"

imp::Framebuffer::Framebuffer()
	: m_Framebuffer()
{
}

imp::Framebuffer::Framebuffer(VkFramebuffer fb)
	: m_Framebuffer(fb)
{
}

void imp::Framebuffer::Destroy(VkDevice device)
{

}
