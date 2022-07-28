#include "Surface.h"
#include <cassert>

imp::Surface::Surface()
{
}

imp::Surface::Surface(Image& img, const SurfaceDesc& desc, uint64_t frameLastUsed)
	: m_Image(img), m_SurfaceDesc(desc), m_FrameLastUsed(frameLastUsed), m_Semaphore(0)
{
}

imp::Image imp::Surface::GetImage() const
{
	return m_Image;
}

const imp::SurfaceDesc& imp::Surface::GetDesc() const
{
	return m_SurfaceDesc;
}

void imp::Surface::UpdateLastUsed(unsigned long long currFrame)
{
	m_FrameLastUsed = currFrame;
}

void imp::Surface::AddSemaphore(VkSemaphore sem)
{
	m_Semaphore = sem;
}
