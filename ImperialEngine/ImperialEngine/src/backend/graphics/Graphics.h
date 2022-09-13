#pragma once
#include "backend/graphics/RenderPassGeneratorBase.h"
#include "backend/graphics/CommandBufferManager.h"
#include "backend/graphics/VulkanShaderManager.h"
#include "backend/graphics/PipelineManager.h"
#include "backend/graphics/SurfaceManager.h"
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/RenderPassImGUI.h"
#include "backend/graphics/RenderPass.h"
#include "backend/graphics/Swapchain.h"
#include "backend/graphics/VkDebug.h"
#include "backend/VulkanGarbageCollector.h"
#include "backend/VulkanMemory.h"
#include "backend/VkWindow.h"
#include "Utils/NonCopyable.h"
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
		bool hasUI;
	};

	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void RenderCameras();
		void RenderImGUI();
		void EndFrame();

		void CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData);
		void CreateAndUploadMaterials(const std::vector<CmdRsc::MaterialCreationRequest>& materialCreationData);

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

		const Pipeline& EnsurePipeline(VkCommandBuffer cb, const RenderPassBase& rp /*, Material material*/);
		void PushConstants(VkCommandBuffer cb, const void* data, uint32_t size, VkPipelineLayout pipeLayout) const;
		// returns index count
		uint32_t BindMesh(VkCommandBuffer cb, uint32_t vtxBufferId) const;
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

		CommandBufferManager m_CbManager;
		SurfaceManager m_SurfaceManager;
		VulkanShaderManager m_ShaderManager;
		PipelineManager m_PipelineManager;
		RenderPassGeneratorBase* m_RenderPassManager;

		VkWindow m_Window;
		VulkanGarbageCollector m_VulkanGarbageCollector;
		VulkanMemory m_MemoryManager;
		MemoryProps m_DeviceMemoryProps;

		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
		VulkanBuffer m_MeshBuffer;

		std::unordered_map<uint32_t, Comp::IndexedVertexBuffers> m_VertexBuffers;

	public:
		// TODO: remove this section

		std::vector<DrawDataSingle> m_DrawData;
		std::vector<CameraData>		m_CameraData;
		//DrawData<DrawDataSingle> m_DrawData;
	private:

		friend class RenderPassBase;
		friend class RenderPass;
		friend class RenderPassImGUI;
		// prototyping..
		//RenderPassBase* renderpass;
		std::shared_ptr<RenderPassBase> renderpassgui;
	};
}