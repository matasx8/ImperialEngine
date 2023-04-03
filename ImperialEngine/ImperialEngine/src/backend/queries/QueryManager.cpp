#include "QueryManager.h"
#include <cassert>

namespace imp
{
	QueryManager::QueryManager()
		: m_Pool(), m_QueryCount()
	{
	}

	void QueryManager::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, const EngineGraphicsSettings& settings)
	{
		const uint32_t fullQueryCount = 128;
		VkQueryPoolCreateInfo ci = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		ci.queryType = VK_QUERY_TYPE_TIMESTAMP;
		ci.queryCount = fullQueryCount;
		ci.flags = 0;

		auto res = vkCreateQueryPool(device, &ci, nullptr, &m_Pool);
		assert(res == VK_SUCCESS);

		m_QueryCount = fullQueryCount / settings.swapchainImageCount;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice, &props);
		assert(props.limits.timestampComputeAndGraphics);

		m_TimestampPeriod = props.limits.timestampPeriod;
	}

	void QueryManager::WriteTimestamp(VkCommandBuffer cb, uint32_t queryTypeIndex, uint32_t swapchainIndex)
	{
		assert(queryTypeIndex < m_QueryCount);
		auto indexWithOffset = queryTypeIndex + swapchainIndex * m_QueryCount;
		vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_Pool, indexWithOffset);

		m_UsedQueries[swapchainIndex][queryTypeIndex] = true;
	}

	void QueryManager::ResetQueries(VkCommandBuffer cb, uint32_t swapchainIndex)
	{
		vkCmdResetQueryPool(cb, m_Pool, swapchainIndex * m_QueryCount, m_QueryCount);
		m_UsedQueries[swapchainIndex].reset();
	}

	void QueryManager::ReadbackQueryResults(VkDevice device, uint32_t swapchainIndex)
	{
		uint64_t results[kQueryCount] = {};

		uint32_t offset = swapchainIndex * m_QueryCount;
		uint32_t firstQuery = swapchainIndex * m_QueryCount;

		vkGetQueryPoolResults(device, m_Pool, firstQuery, kQueryCount, sizeof(results), results, sizeof(results[0]), VK_QUERY_RESULT_64_BIT);
		
		auto& usedQueries = m_UsedQueries[swapchainIndex];
		if (usedQueries.any())
		{
			double beginQ = 0.0;
			double endQ = 0.0;
			std::string msg;

			for (uint32_t i = 0; i < kQueryCount; i++)
			{
				if (usedQueries[i])
				{
					switch ((QueryType)i)
					{
						case kQueryGPUFrameBegin:
						{
							msg = "GPU Frame TIME %f ms\n";
							beginQ = results[kQueryGPUFrameBegin];
							break;
						}
						case kQueryGPUFrameEnd:
						{
							endQ = results[kQueryGPUFrameEnd];
							break;
						}
						case kQueryCullBegin:
						{
							msg = "GPU CULL TIME %f ms\n";
							beginQ = results[kQueryCullBegin];
							break;
						}
						case kQueryCullEnd:
						{
							endQ = results[kQueryCullEnd];
							break;
						}
						case kQueryDrawBegin:
						{
							msg = "GPU DRAW TIME %f ms\n";
							beginQ = results[kQueryDrawBegin];
							break;
						}
						case kQueryDrawEnd:
						{
							endQ = results[kQueryDrawEnd];
							break;
						}
						default:
							break;
					}

					if (i % 2 != 0)
					{
						//printf(msg.c_str(), (endQ - beginQ) * m_TimestampPeriod * 1e-6);
					}
				}
			}
		}
	}

	void QueryManager::Destroy(VkDevice device)
	{
		vkDestroyQueryPool(device, m_Pool, nullptr);
	}
}