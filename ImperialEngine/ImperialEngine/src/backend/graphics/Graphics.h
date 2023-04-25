#pragma once
#include "backend/graphics/RenderPassGeneratorBase.h"
#include "backend/graphics/CommandBufferManager.h"
#include "backend/graphics/VulkanShaderManager.h"
#include "backend/graphics/PipelineManager.h"
#include "backend/graphics/SurfaceManager.h"
#include "backend/graphics/RenderPassImGUI.h"
#include "backend/graphics/Semaphore.h"
#include "backend/graphics/Swapchain.h"
#include "backend/graphics/VkDebug.h"
#include "backend/queries/QueryManager.h"
#include "backend/VariousTypeDefinitions.h"
#include "backend/VulkanGarbageCollector.h"
#include "backend/VulkanMemory.h"
#include "backend/VkWindow.h"
#include "Utils/Pool.h"
#include "Utils/SimpleTimer.h"
#include "Utils/FrameTimeTable.h"
#include "frontend/Components/Components.h"
#include <extern/ENTT/entt.hpp>
#include <barrier>

#include "extern/AFTERMATH/NsightAftermathGpuCrashTracker.h"

namespace BS { class thread_pool; }

namespace imp
{
	class Window;
	namespace CmdRsc { struct MeshCreationRequest; }

	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void DoTransfers(bool releaseAll);
		void UpdateDrawCommands();
		void Cull();
		void StartFrame();
		void RenderCameras();
		void RenderImGUI();
		void EndFrame();

#if BENCHMARK_MODE
		void StartBenchmark();
		void StopBenchmark();
		const std::array<FrameTimeTable, kEngineRenderModeCount>& GetBenchmarkTable() const;
#endif

		void CreateAndUploadMeshes(std::vector<MeshCreationRequest>& meshCreationData);
		void CreateAndUploadMaterials(const std::vector<MaterialCreationRequest>& materialCreationData);
		void CreateComputePrograms(const std::vector<ComputeProgramCreationRequest>& computeProgramRequests);

		// Will return ref to VulkanBuffer used for uploading new draw commands.
		// Actual data is indices to "MeshData" that compute can use to generate actual commands.
		// Waits for fence associated with buffer to make sure it's not used by the GPU anymore.
		IGPUBuffer& GetDrawCommandStagingBuffer();
		// Will return ref to VulkanBuffer used for uploading new descriptor draw data
		IGPUBuffer& GetDrawDataBuffer();

		const Comp::MeshGeometry& GetMeshData(uint32_t index) const;

		Timings& GetFrameTimings() { return m_Timer; }
		Timings& GetOldFrameTimings() { return m_OldTimer; }
		SimpleTimer& GetSyncTimings() { return m_SyncTimer; }
		SimpleTimer& GetOldSyncTimings() { return m_SyncTimer; }

		EngineGraphicsSettings& GetGraphicsSettings();
		const GraphicsCaps& GetGfxCaps() const;

		void Destroy();

	private:

		void CreateInstance();
		void FindPhysicalDevice();
		void CreateLogicalDevice();
		void CreateVkWindow(Window* window);
		void CreateSwapchain();
		void CreateCommandBufferManager();
		void CreateSurfaceManager();
		void CreateGarbageCollector();
		void CreateImGUI();
		void CreateVulkanMemoryManager();
		void CreateRenderPassGenerator();
		
		void InitializeVulkanMemory();

		// transfer commands
		VulkanBuffer UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkBufferUsageFlags dstUsageFlags, VkMemoryPropertyFlags memoryFlags, VkMemoryPropertyFlags dstMemoryFlags, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload);
		void UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VulkanBuffer& dst, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload);
		void CopyVulkanBuffer(const VulkanBuffer& src, VulkanBuffer& dst, const CommandBuffer& cb);

		void AcquireDrawCommandBuffer(CommandBuffer& cb);

		const Pipeline& EnsurePipeline(VkCommandBuffer cb, const RenderPass& rp /*, Material material*/);
		void PushConstants(VkCommandBuffer cb, const void* data, uint32_t size, VkPipelineLayout pipeLayout) const;

		void DrawIndexed(VkCommandBuffer cb, uint32_t indexCount) const;

		const VulkanBuffer& GetDrawCommandCountBuffer();
		
		bool CheckExtensionsSupported(std::vector<const char*>& extensions) const;

		EngineGraphicsSettings m_Settings;
		GraphicsCaps m_GfxCaps;
		ValidationLayers m_ValidationLayers;

		VkInstance m_VkInstance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;

		// These are here for now
		// should save the indices too
		VkQueue m_GfxQueue;
		VkQueue m_TransferQueue;
		VkQueue m_PresentationQueue;

		Swapchain m_Swapchain;
		uint64_t m_CurrentFrame;

		VulkanGarbageCollector m_VulkanGarbageCollector;
		CommandBufferManager m_CbManager;
		CommandBufferManager m_TransferCbManager;
		SurfaceManager m_SurfaceManager;
		VulkanShaderManager m_ShaderManager;
		PipelineManager m_PipelineManager;
		RenderPassGenerator m_RenderPassManager;
		QueryManager m_TimestampQueryManager;

		// TODO benchmark: remove these and replace with benchmark_mode added timers
		Timings m_Timer;
		Timings m_OldTimer;
		SimpleTimer m_SyncTimer; // time spent waiting for gpu
		SimpleTimer m_OldSyncTimer;

#if BENCHMARK_MODE
		void CollectFrameCPUResults();
		void CollectFinalResults();
		std::array<FrameTimeTable, kEngineRenderModeCount> m_FrameTimeTables;
		SimpleTimer m_FrameTimer;
		SimpleTimer m_CullTimer;
		bool m_CollectBenchmarkData;
		uint64_t m_FrameStartedCollecting;
		uint64_t m_FrameStoppedCollecting;
		EngineRenderMode m_EngineRenderModeCollectingInto;
#endif

		// Pools
		PrimitivePool<Semaphore, SemaphoreFactory> m_SemaphorePool;
		PrimitivePool<Fence, FenceFactory> m_FencePool;

		BS::thread_pool* m_JobSystem;

		VkWindow m_Window;
		VulkanMemory m_MemoryManager;
		MemoryProps m_DeviceMemoryProps;

		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
		VulkanBuffer m_DrawBuffer; // buffer with indirect draw commands that indirect draw command will read
		std::array<VulkanBuffer, kEngineSwapchainDoubleBuffering> m_StagingDrawBuffer;
		VulkanBuffer m_BoundingVolumeBuffer;
		uint32_t m_NumDraws;

		std::array<VulkanBuffer, kEngineSwapchainDoubleBuffering> m_GlobalBuffers;
		std::array<VkDescriptorSet, kEngineSwapchainDoubleBuffering> m_DescriptorSets;

		GpuCrashTracker m_AfterMathTracker;

	public:
		std::unordered_map<uint32_t, Comp::MeshGeometry> m_VertexBuffers;

		std::unordered_map<uint32_t, BoundingVolumeSphere> m_BVs;

		// TODO: remove this section and replace with some API

		std::vector<DrawDataSingle> m_DrawData;
		CameraData m_MainCamera;
		CameraData m_PreviewCamera;

		// Temporary workaround to know whether to do transfer in StartFrame or in UpdateDraws
		bool m_DelayTransferOperation;
	private:

		friend class RenderPass;
		friend class RenderPassImGUI;
		friend class DefaultColorRP;
		friend class DefaultDepthRP;
		std::shared_ptr<RenderPass> renderpassgui;
	};
}