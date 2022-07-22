#include "Surface.h"

imp::Surface::Surface()
{
}

imp::Surface::Surface(Image img, SurfaceDesc& desc, uint64_t frameLastUsed)
{
}

imp::Image imp::Surface::GetImage() const
{
	return Image();
}

const imp::SurfaceDesc& imp::Surface::GetDesc() const
{
	// // O: insert return statement here
	return {};
}

void imp::Surface::UpdateLastUsed(unsigned long long currFrame)
{
}
