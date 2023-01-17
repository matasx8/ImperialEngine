#pragma once
#include "backend/graphics/RenderPassGeneratorBase.h"
#include "backend/graphics/CommandBufferManager.h"
#include "backend/graphics/VulkanShaderManager.h"
#include "backend/graphics/PipelineManager.h"
#include "backend/graphics/SurfaceManager.h"
#include "backend/graphics/RenderPassImGUI.h"
#include "backend/graphics/Swapchain.h"
#include "backend/graphics/VkDebug.h"
#include "backend/VulkanGarbageCollector.h"
#include "backend/VulkanMemory.h"
#include "backend/VkWindow.h"
#include "frontend/Components/Components.h"
#include <extern/ENTT/entt.hpp>
#include <barrier>

namespace imp
{
	class Window;
	namespace CmdRsc { struct MeshCreationRequest; }

	struct DrawDataSingle
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


	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void StartFrame();
		void RenderCameras();
		void RenderImGUI();
		void EndFrame();

		void CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData);
		void CreateAndUploadMaterials(const std::vector<CmdRsc::MaterialCreationRequest>& materialCreationData);

		EngineGraphicsSettings& GetGraphicsSettings();

		void Destroy();

		static VkSemaphore GetSemaphore(VkDevice device);

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
		VkQueue m_PresentationQueue;

		Swapchain m_Swapchain;
		uint64_t m_CurrentFrame;

		VulkanGarbageCollector m_VulkanGarbageCollector;
		CommandBufferManager m_CbManager;
		SurfaceManager m_SurfaceManager;
		VulkanShaderManager m_ShaderManager;
		PipelineManager m_PipelineManager;
		RenderPassGenerator m_RenderPassManager;

		VkWindow m_Window;
		VulkanMemory m_MemoryManager;
		MemoryProps m_DeviceMemoryProps;

		// can i use any of the entt data structures?
		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
		VulkanBuffer m_MeshBuffer;
		VulkanBuffer m_DrawBuffer;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_GlobalBuffers;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_DrawDataBuffers;
		std::array<VkDescriptorSet, kEngineSwapchainExclusiveMax - 1> m_DescriptorSets;

		std::unordered_map<uint32_t, Comp::IndexedVertexBuffers> m_VertexBuffers;

	public:
		// TODO: remove this section

		std::vector<DrawDataSingle> m_DrawData;
		std::vector<CameraData>		m_CameraData;
		//DrawData<DrawDataSingle> m_DrawData;
	private:

		friend class RenderPass;
		friend class RenderPassImGUI;
		friend class DefaultColorRP;
		friend class DefaultDepthRP;
		// prototyping..
		//RenderPass* renderpass;
		std::shared_ptr<RenderPass> renderpassgui;
	};
}