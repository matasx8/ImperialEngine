#pragma once
#include "Utils/NonCopyable.h"
#include "frontend/EngineSettings.h"
#include "Utils/EngineStaticConfig.h"
#include "volk.h"
#include <bitset>
#include <array>

namespace imp
{
	class FrameTimeTable;

	enum QueryType : uint32_t
	{
		kQueryGPUFrameBegin,
		kQueryGPUFrameEnd,
		kQueryCullBegin,
		kQueryCullEnd,
		kQueryDrawBegin,
		kQueryDrawEnd,
		kQueryCount
	};

	class QueryManager : NonCopyable
	{
	public:
		QueryManager();

		void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, const EngineGraphicsSettings& settings);

		void WriteTimestamp(VkCommandBuffer cb, uint32_t queryTypeIndex, uint32_t swapchainIndex);
		void ResetQueries(VkCommandBuffer cb, uint32_t swapchainIndex);
#if BENCHMARK_MODE
		void ReadbackQueryResults(VkDevice device, FrameTimeTable& table, uint64_t currFrame, uint64_t frameStartedCollecting, uint32_t swapchainIndex);
#else
		void ReadbackQueryResults(VkDevice device, uint32_t swapchainIndex);
#endif

		void Destroy(VkDevice device);

	private:

		VkQueryPool m_Pool;
		uint32_t m_QueryCount;
		float m_TimestampPeriod;

		std::array<std::bitset<kQueryCount>, kEngineSwapchainDoubleBuffering> m_UsedQueries;
	};
}