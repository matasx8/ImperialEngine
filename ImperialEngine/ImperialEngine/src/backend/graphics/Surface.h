#pragma once
#include "Image.h"
#include <vcruntime_string.h>
#include <functional>

namespace imp
{
	struct SurfaceDesc
	{
		uint32_t width;
		uint32_t height;
		VkFormat format;
		uint32_t msaaCount;
		uint32_t finalLayout;
		bool isColor;
		bool isBackbuffer;

		inline bool operator==(const SurfaceDesc& other) const noexcept
		{
			return memcmp(this, &other, sizeof(SurfaceDesc)) == 0;
		}

		inline bool operator!=(const SurfaceDesc& other) const noexcept
		{
			return memcmp(this, &other, sizeof(SurfaceDesc)) != 0;
		}
	};

	class Surface
	{
	public:
		Surface();
		Surface(Image& img, const SurfaceDesc& desc, uint64_t frameLastUsed);

		Image GetImage() const;
		const SurfaceDesc& GetDesc() const;
		void UpdateLastUsed(unsigned long long currFrame);
		void AddSemaphore(VkSemaphore sem);
		VkSemaphore GetSemaphore();

	private:
		Image m_Image;
		SurfaceDesc m_SurfaceDesc;
		uint64_t m_FrameLastUsed;
		VkSemaphore m_Semaphore;
	};
}


template <>
struct std::hash<imp::SurfaceDesc>
{
	std::size_t operator()(const imp::SurfaceDesc& k) const
	{
		return (std::hash<uint32_t>()(k.width) ^ ((std::hash<uint32_t>()(k.height) << 1)) >> 1) ^
			(std::hash<uint32_t>()(k.format) ^ ((std::hash<uint32_t>()(k.msaaCount) << 1)) >> 1) ^
			(std::hash<uint32_t>()(k.finalLayout) ^ ((std::hash<bool>()(k.isColor) << 1)) >> 1) ^
			(std::hash<bool>()(k.isBackbuffer) << 1);
	}
};