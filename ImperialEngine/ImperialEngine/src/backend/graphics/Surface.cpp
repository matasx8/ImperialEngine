#include "Surface.h"

imp::Surface::Surface()
{
}

imp::Surface::Surface(Image& img, SurfaceDesc& desc, uint64_t frameLastUsed)
	: m_Image(img), m_SurfaceDesc(desc), m_FrameLastUsed(frameLastUsed)
{
}

imp::Image imp::Surface::GetImage() const
{
	return m_Image;
}

const imp::SurfaceDesc& imp::Surface::GetDesc() const
{
	// // O: insert return statement here
	return {};
}

void imp::Surface::UpdateLastUsed(unsigned long long currFrame)
{
}
