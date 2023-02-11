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
#include "backend/VulkanGarbageCollector.h"
#include "backend/VulkanMemory.h"
#include "backend/VkWindow.h"
#include "Utils/Pool.h"
#include "Utils/SimpleTimer.h"
#include "frontend/Components/Components.h"
#include <extern/ENTT/entt.hpp>
#include <barrier>

namespace imp
{
	class Window;
	namespace CmdRsc { struct MeshCreationRequest; }

	struct alignas(16) DrawDataSingle
	{
		glm::mat4x4 Transform;
		uint32_t VertexBufferId;
	};
	struct CameraData
	{
		glm::mat4x4 Projection;
		glm::mat4x4 View;
		uint32_t camOutputType;
		uint32_t cameraID;
		bool dirty;
		bool hasUI;
	};

	// TODO acceleration-part-1: move these somewhere
	struct IndirectDrawCmd
	{
		// VkDrawIndexedIndirectCommand:
		uint32_t    indexCount;
		uint32_t    instanceCount;
		uint32_t    firstIndex;
		int32_t     vertexOffset;
		uint32_t    firstInstance;
		// custom:
		uint32_t	boundingVolumeIndex; // index to bounding volume descriptor
	};


	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void DoTransfers(bool releaseAll);
		void UpdateDrawCommands();
		void GraphicsQueueStart();
		void Cull();
		void StartFrame();
		void RenderCameras();
		void RenderImGUI();
		void EndFrame();

		void CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData);
		void CreateAndUploadMaterials(const std::vector<CmdRsc::MaterialCreationRequest>& materialCreationData);
		void CreateComputePrograms(const std::vector<CmdRsc::ComputeProgramCreationRequest>& computeProgramRequests);

		// Will return ref to VulkanBuffer used for uploading new draw commands.
		// Waits for fence associated with buffer to make sure it's not used by the GPU anymore.
		IGPUBuffer& GetDrawCommandStagingBuffer();
		// Will return ref to VulkanBuffer used for uploading new descriptor draw data
		IGPUBuffer& GetDrawDataBuffer();

		const Comp::IndexedVertexBuffers& GetMeshData(uint32_t index) const;

		Timings& GetFrameTimings() { return m_Timer; }
		Timings& GetOldFrameTimings() { return m_OldTimer; }
		SimpleTimer& GetSyncTimings() { return m_SyncTimer; }
		SimpleTimer& GetOldSyncTimings() { return m_SyncTimer; }

		EngineGraphicsSettings& GetGraphicsSettings();

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

		const Pipeline& EnsurePipeline(VkCommandBuffer cb, const RenderPass& rp /*, Material material*/);
		void PushConstants(VkCommandBuffer cb, const void* data, uint32_t size, VkPipelineLayout pipeLayout) const;

		void DrawIndexed(VkCommandBuffer cb, uint32_t indexCount) const;

		const VulkanBuffer& GetDrawCommandCountBuffer();
		
		bool CheckExtensionsSupported(std::vector<const char*> extensions);

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

		Timings m_Timer;
		Timings m_OldTimer;
		SimpleTimer m_SyncTimer; // time spent waiting for gpu
		SimpleTimer m_OldSyncTimer;

		// Pools
		PrimitivePool<Semaphore, SemaphoreFactory> m_SemaphorePool;
		PrimitivePool<Fence, FenceFactory> m_FencePool;

		VkWindow m_Window;
		VulkanMemory m_MemoryManager;
		MemoryProps m_DeviceMemoryProps;

		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
		VulkanBuffer m_MeshBuffer;
		VulkanBuffer m_DrawBuffer;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_StagingDrawBuffer;
		//std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_StagingDrawDataBuffer;
		VulkanBuffer m_BoundingVolumeBuffer;
		uint32_t m_NumDraws;

		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_GlobalBuffers;
		std::array<VkDescriptorSet, kEngineSwapchainExclusiveMax - 1> m_DescriptorSets;

	public:
		std::unordered_map<uint32_t, Comp::IndexedVertexBuffers> m_VertexBuffers;

		// TODO: remove this section and replace with some API

		std::vector<DrawDataSingle> m_DrawData;
		std::vector<CameraData>		m_CameraData;

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