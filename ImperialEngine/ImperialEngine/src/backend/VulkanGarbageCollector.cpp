#include "VulkanGarbageCollector.h"

imp::VulkanGarbageCollector::VulkanGarbageCollector()
	: m_SafeFrames(3ull)
{
}

void imp::VulkanGarbageCollector::Initialize(uint64_t safeFrames)
{
	m_SafeFrames = safeFrames;
}

void imp::VulkanGarbageCollector::AddGarbageResource(std::shared_ptr<VulkanResource> res)
{
	m_GarbageQueue.push(res);
}

void imp::VulkanGarbageCollector::DestroySafeResources(VkDevice device, uint64_t currentFrame)
{
	for (int i = 0; i < m_GarbageQueue.size(); i++)
	{
		auto res = m_GarbageQueue.front().get();
		const auto diff = currentFrame - res->GetLastUsed();
		if (diff < m_SafeFrames)
			return;
		
		res->Destroy(device);
		m_GarbageQueue.pop();
	}
}

void imp::VulkanGarbageCollector::DestroyAllImmediate(VkDevice device)
{
}
