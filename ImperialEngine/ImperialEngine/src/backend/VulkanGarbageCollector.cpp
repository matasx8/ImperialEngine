#include "VulkanGarbageCollector.h"

imp::VulkanGarbageCollector::VulkanGarbageCollector()
	: m_SafeFrames(3ull)
{
}

void imp::VulkanGarbageCollector::Initialize(uint64_t safeFrames)
{
	m_SafeFrames = safeFrames + 5;
}

void imp::VulkanGarbageCollector::AddGarbageResource(std::shared_ptr<VulkanResource> res)
{
	m_GarbageQueue.push(res);
}

void imp::VulkanGarbageCollector::DestroySafeResources(VkDevice device, uint64_t currentFrame)
{
	const auto size = m_GarbageQueue.size();
	for (auto i = 0; i < size; i++)
	{
		auto& res = m_GarbageQueue.front();
		const auto diff = currentFrame - res->GetLastUsed();
		if (diff < m_SafeFrames)
			return;
		
		res->Destroy(device);
		m_GarbageQueue.pop();
	}
}

void imp::VulkanGarbageCollector::DestroyAllImmediate(VkDevice device)
{
	const auto size = m_GarbageQueue.size();
	for (auto i = 0; i < size; i++)
	{
		m_GarbageQueue.front()->Destroy(device);
		m_GarbageQueue.pop();
	}
}
