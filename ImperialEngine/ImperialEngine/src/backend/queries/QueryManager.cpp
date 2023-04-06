#include "QueryManager.h"
#include "Utils/FrameTimeTable.h"
#include <cassert>

namespace imp
{
	QueryManager::QueryManager()
		: m_Pool(), m_StatPool(), m_QueryCount(), m_TimestampPeriod(), m_UsedQueries()
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

		ci.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		ci.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
		res = vkCreateQueryPool(device, &ci, nullptr, &m_StatPool);
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

	void QueryManager::BeginPipelineStatQueries(VkCommandBuffer cb, uint32_t swapchainIndex)
	{
		const auto offset = swapchainIndex * m_QueryCount;
		vkCmdBeginQuery(cb, m_StatPool, kStatQueryTriangles + offset, 0);
	}

	void QueryManager::EndPipelineStatQueries(VkCommandBuffer cb, uint32_t swapchainIndex)
	{
		const auto offset = swapchainIndex * m_QueryCount;
		vkCmdEndQuery(cb, m_StatPool, kStatQueryTriangles + offset);
	}

	void QueryManager::ResetQueries(VkCommandBuffer cb, uint32_t swapchainIndex)
	{
		vkCmdResetQueryPool(cb, m_StatPool, swapchainIndex * m_QueryCount, m_QueryCount);
		vkCmdResetQueryPool(cb, m_Pool, swapchainIndex * m_QueryCount, m_QueryCount);
		m_UsedQueries[swapchainIndex].reset();
	}

#if BENCHMARK_MODE
	void QueryManager::ReadbackQueryResults(VkDevice device, FrameTimeTable& table, uint64_t currFrame, uint64_t frameStartedCollecting, uint32_t swapchainIndex)
#else
	void QueryManager::ReadbackQueryResults(VkDevice device, uint32_t swapchainIndex)
#endif
	{
#if BENCHMARK_MODE
		// TODO kEngineSwapchainDoubleBuffering: assuming now that we're only using this
		// should be getting results for frame n - 2
		if (currFrame - frameStartedCollecting < kEngineSwapchainDoubleBuffering)
			return;

		const auto currIdx = currFrame - frameStartedCollecting;
		const auto actualIdx = currIdx - kEngineSwapchainDoubleBuffering;
		auto& row = table.table_rows[actualIdx];
#endif
		uint64_t results[kQueryCount] = {};
		uint64_t statResults[kStatQueryCount] = {};

		//uint32_t offset = swapchainIndex * m_QueryCount;
		uint32_t firstQuery = swapchainIndex * m_QueryCount;

		vkGetQueryPoolResults(device, m_Pool, firstQuery, kQueryCount, sizeof(results), results, sizeof(results[0]), VK_QUERY_RESULT_64_BIT);
		vkGetQueryPoolResults(device, m_StatPool, firstQuery, kStatQueryCount, sizeof(statResults), statResults, sizeof(statResults[0]), VK_QUERY_RESULT_64_BIT);
		
#if BENCHMARK_MODE
		row.triangles = uint64_t(statResults[0]);
#endif

		//printf("Num Triangles %llu\n", statResults[0]);
		auto& usedQueries = m_UsedQueries[swapchainIndex];
		if (usedQueries.any())
		{
			double beginQ = 0.0;
			double endQ = 0.0;

			for (uint32_t i = 0; i < kQueryCount; i++)
			{

				if (usedQueries[i])
				{
					if (i % 2 == 0) // begin query
						beginQ = results[i];
					else			// end query
						endQ = results[i];

					switch ((QueryType)i)
					{
						case kQueryGPUFrameEnd:
						{
#if BENCHMARK_MODE
							row.frameGPU = (endQ - beginQ) * 1e-6;
#endif
							break;
						}
						case kQueryCullEnd:
						{
#if BENCHMARK_MODE
							row.cull = (endQ - beginQ) * 1e-6;
#endif
							break;
						}
						default:
							break;
					}
				}
			}
		}
	}

	void QueryManager::Destroy(VkDevice device)
	{
		vkDestroyQueryPool(device, m_Pool, nullptr);
		vkDestroyQueryPool(device, m_StatPool, nullptr);
	}
}