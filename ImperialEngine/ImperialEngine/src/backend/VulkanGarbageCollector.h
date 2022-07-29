#pragma once
#include "backend/VulkanResource.h"
#include "Utils/NonCopyable.h"
#include <queue>
#include <functional>

namespace imp
{
	class VulkanGarbageCollector : NonCopyable
	{
	public:
		VulkanGarbageCollector();
		void Initialize(uint64_t safeFrames);

		void AddGarbageResource(std::shared_ptr<VulkanResource> res);

		void DestroySafeResources(VkDevice device, uint64_t currentFrame);
		void DestroyAllImmediate(VkDevice device);


	private:
		uint64_t m_SafeFrames;
		std::queue<std::shared_ptr<VulkanResource>> m_GarbageQueue;

	};
}