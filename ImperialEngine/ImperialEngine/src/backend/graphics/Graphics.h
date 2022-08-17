#pragma once
#include "backend/graphics/CommandBufferManager.h"
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

	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void PrototypeRenderPass();
		void RenderImGUI();
		void EndFrame();

		void CreateAndUploadMeshes(const std::vector<CmdRsc::MeshCreationRequest>& meshCreationData);

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
		

		// transfer commands
		VulkanBuffer UploadVulkanBuffer(VkBufferUsageFlags usageFlags, VkBufferUsageFlags dstUsageFlags, VkMemoryPropertyFlags memoryFlags, VkMemoryPropertyFlags dstMemoryFlags, const CommandBuffer& cb, uint32_t allocSize, const void* dataToUpload);
		void CopyVulkanBuffer(const VulkanBuffer& src, VulkanBuffer& dst, const CommandBuffer& cb);

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

		VkWindow m_Window;
		VulkanGarbageCollector m_VulkanGarbageCollector;
		VulkanMemory m_MemoryManager;
		MemoryProps m_DeviceMemoryProps;

		std::unordered_map<uint32_t, Comp::IndexedVertexBuffers> m_VertexBuffers;

		// On the other side, a system that has access to a view or a group can only iterate, read and update entities and components.
		// only have a group or view? Dunno if that would work threaded
	public:
		//entt::registry m_GfxEntities;
		struct DrawDataSingle
		{
			glm::mat4x4 Transform;
			uint32_t VertexBufferId;
		};
		std::vector<DrawDataSingle> m_DrawData;
		//DrawData<DrawDataSingle> m_DrawData;
	private:

		friend class RenderPassBase;
		friend class RenderPass;
		friend class RenderPassImGUI;
		// prototyping..
		RenderPassBase* renderpass;
		RenderPassBase* renderpassgui;
	};
}