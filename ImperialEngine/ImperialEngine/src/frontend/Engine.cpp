#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Engine.h"
#include "Utils/GfxUtilities.h"
#include "Components/Components.h"
#include "extern/IMGUI/imgui.h"
#include "extern/THREAD-POOL/BS_thread_pool.hpp"
#include "extern/GLM/ext/matrix_transform.hpp"
#include "extern/GLM/ext/matrix_clip_space.hpp"
#include "extern/GLM/gtx/quaternion.hpp"
#include "Utils/EngineStaticConfig.h"
#include <barrier>
#include <execution>

namespace imp
{
	Engine::Engine()
		: m_Entities()
		, m_DrawDataDirty(false)
		, m_Q(nullptr)
		, m_Worker(nullptr)
		, m_SyncPoint(nullptr)
		, m_EngineSettings()
		, m_Window()
		, m_UI()
		, m_ThreadPool(nullptr)
		, m_Gfx()
		, m_VisibleDrawData()
#if BENCHMARK_MODE
		, m_InitialCameraTransform()
		, m_FrameTimeTables()
		, m_FrameTimer()
		, m_CullTimer()
		, m_FullFrameTimer()
		, m_LastFrameTime()
		, m_BenchmarkDone()
		, m_CollectBenchmarkData()
#endif
		, m_AssetImporter(*this)
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
		m_EngineSettings = settings;
		InitThreading(m_EngineSettings.threadingMode);
#if !BENCHMARK_MODE
		InitImgui();
#endif
		InitWindow();
		InitGraphics();
		CreateCameras();

		// wait until backend initted
		m_SyncPoint->arrive_and_wait();
		return true;
	}

	void Engine::LoadScenes(const std::vector<std::string>& scenes)
	{
		m_AssetImporter.LoadScenes(scenes);
	}

	void Engine::LoadAssets()
	{
		m_AssetImporter.LoadMaterials("Shaders/spir-v");
		m_AssetImporter.LoadComputeProgams("Shaders/spir-v");
		LoadDefaultStuff();
		MarkDrawDataDirty();
	}

	void Engine::DistributeEntities(const std::string& distribution, const std::string& entityCount)
	{
		uint32_t numEntities = 0;
		if (entityCount == "max")
			numEntities = kMaxDrawCount;
		else
			numEntities = uint32_t(std::min(std::stoul(entityCount), static_cast<unsigned long>(kMaxDrawCount)));

		if (distribution == "random")
			AddDemoEntity(numEntities);
		else
			printf("[Entity Distribution] Error, trying to use unsupported distribution '%s'\n", distribution.c_str());
	}

	void Engine::StartFrame()
	{
#if BENCHMARK_MODE
		m_FullFrameTimer.stop();
		m_LastFrameTime = m_FullFrameTimer.miliseconds();
		m_FullFrameTimer.start();

		if (m_CollectBenchmarkData)
		{
			m_FrameTimer.start();
		}
		else
		{

		}
#endif
		m_Q->add(std::mem_fn(&Engine::Cmd_StartFrame), std::shared_ptr<void>());
	}

	void Engine::Update()
	{
		// not sure if this should be here
#if !BENCHMARK_MODE
		m_Window.UpdateImGUI();
#endif
		m_Window.Update();
		UpdateRegistry();

		if (m_EngineSettings.gfxSettings.renderMode == kEngineRenderModeTraditional)
		{
			Cull();
		}
	}

	void Engine::Render()
	{
		RenderCameras();
#if !BENCHMARK_MODE
		RenderImGUI();
#endif
	}

	void Engine::EndFrame()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_EndFrame), std::shared_ptr<void>());
	}

	void Engine::SyncRenderThread()
	{
#if BENCHMARK_MODE
		if (m_CollectBenchmarkData)
		{
			m_FrameTimer.stop();

			const auto renderMode = GetCurrentRenderMode();

			FrameTimeRow row;
			row.frameMainCPU = m_FrameTimer.miliseconds();
			row.frame = m_LastFrameTime;

			if (renderMode == kEngineRenderModeTraditional)
				row.cull = m_CullTimer.miliseconds();

			const auto tableIndex = static_cast<uint32_t>(renderMode);
			auto& table = m_FrameTimeTables[tableIndex];
			table.table_rows.push_back(row);
		}
#endif
		if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
			m_Q->add(std::mem_fn(&Engine::Cmd_SyncRenderThread), std::shared_ptr<void>());
	}

	void Engine::SyncGameThread()
	{
		auto& frameWorkTime = m_Timer.frameWorkTime;
		auto& totalTime = m_Timer.totalFrameTime;
		auto& waitTime = m_Timer.waitTime;

		frameWorkTime.stop();

		m_SyncPoint->arrive_and_wait();

		totalTime.stop();

		// time spent waiting for other thread is 'total frame time' - 'sync time'
		waitTime.elapsed_time = totalTime.elapsed_time - m_SyncTime.elapsed_time;

		m_OldTimer = m_Timer;
		m_OldSyncTime = m_SyncTime;

		// start the timer again
		m_Timer.StartAll();
	}

#if BENCHMARK_MODE
	void Engine::StartBenchmark()
	{
		m_CollectBenchmarkData = true;

		m_Q->add(std::mem_fn(&Engine::Cmd_StartBenchmark), std::shared_ptr<void>());
	}

	void Engine::StopBenchmark()
	{
		m_CollectBenchmarkData = false;

		auto& reg = GetEntityRegistry();
		const auto cameras = reg.view<Comp::Transform, Comp::Camera>();

		for (auto ent : cameras)
		{
			auto& transform = cameras.get<Comp::Transform>(ent);
			transform.transform = m_InitialCameraTransform;
		}

		m_Q->add(std::mem_fn(&Engine::Cmd_StopBenchmark), std::shared_ptr<void>());
	}

	const std::array<FrameTimeTable, kEngineRenderModeCount>& Engine::GetMainBenchmarkTable() const
	{
		return m_FrameTimeTables;
	}

	const std::array<FrameTimeTable, kEngineRenderModeCount>& Engine::GetRenderBenchmarkTable() const
	{
		return m_Gfx.GetBenchmarkTable();
	}
#endif

	entt::registry& Engine::GetEntityRegistry()
	{
		return m_Entities;
	}

	bool Engine::IsCurrentRenderMode(EngineRenderMode mode) const
	{
		return m_EngineSettings.gfxSettings.renderMode == mode;
	}

	EngineRenderMode Engine::GetCurrentRenderMode() const
	{
		return m_EngineSettings.gfxSettings.renderMode;
	}

	bool Engine::IsRenderingModeSupported(EngineRenderMode mode) const
	{
		if (mode == kEngineRenderModeGPUDrivenMeshShading)
		{
			const auto& caps = m_Gfx.GetGfxCaps();
			if (!caps.IsMeshShadingSupported())
				return false;
		}
		return true;
	}

	void Engine::SwitchRenderingMode(EngineRenderMode newRenderMode)
	{
		if (newRenderMode == kEngineRenderModeGPUDrivenMeshShading)
		{
			const auto& caps = m_Gfx.GetGfxCaps();
			if (!caps.IsMeshShadingSupported())
				newRenderMode = kEngineRenderModeGPUDriven;
		}

		m_EngineSettings.gfxSettings.renderMode = newRenderMode;
		m_Q->add(std::mem_fn(&Engine::Cmd_ChangeRenderMode), std::make_shared<EngineRenderMode>(newRenderMode));
		MarkDrawDataDirty();
	}

	void Engine::AddDemoEntity(uint32_t count)
	{
		if (count)
			MarkDrawDataDirty();

		const auto numMeshes = m_AssetImporter.GetNumberOfUniqueMeshesLoaded();

		auto& reg = m_Entities;
		for (auto i = 0; i < count; i++)
		{
			const auto monkey = reg.create();
			const auto identityMat = glm::mat4x4(1.0f);
			const auto randomOffset = glm::vec3((float)(rand() % 80 - 40), (float)(rand() % 80 - 40), (float)(rand() % 80 - 40));
			const auto randomRot = glm::vec3((float)(rand()), (float)(rand()), (float)(rand()));
			const auto newMatrix = glm::translate(identityMat, randomOffset);
			const auto nnn = glm::rotate(newMatrix, (float)(rand() % 360), glm::normalize(randomRot));
			reg.emplace<Comp::Transform>(monkey, nnn);
			
			const auto child = reg.create();
			reg.emplace<Comp::ChildComponent>(child, monkey);
			reg.emplace<Comp::Mesh>(child, (uint32_t)rand() % numMeshes);
			//reg.emplace<Comp::Mesh>(child, (uint32_t)0);
			reg.emplace<Comp::Material>(child, kDefaultMaterialIndex);
		}
	}

	bool Engine::ShouldClose() const
	{
#if !BENCHMARK_MODE
		return m_Window.ShouldClose();
#else
		return m_Window.ShouldClose() || m_BenchmarkDone;
#endif
	}

	void Engine::ShutDown()
	{
		CleanUpThreading();
		CleanUpWindow();
		CleanUpGraphics();
#if !BENCHMARK_MODE
		CleanUpUI();
#endif
	}

	void Engine::InitThreading(EngineThreadingMode mode)
	{
		BarrierFunctionObject func(*this);
		switch (mode)
		{
		case kEngineSingleThreaded:
			// TODO: I may have to abandon this..
			// single-threaded so we only need the single-threaded version of the queue that executes task on add.
			m_Q = new prl::WorkQ_ST<Engine>(*this);

			m_SyncPoint = new std::barrier(1, func);
			break;
		case kEngineMultiThreaded:
			constexpr int numThreads = 2;
			m_Q = new prl::WorkQ<Engine>();
			m_Worker = new prl::ConsumerThread<Engine>(*this, *m_Q);
			m_SyncPoint = new std::barrier(numThreads, func);
			m_ThreadPool = new BS::thread_pool(std::thread::hardware_concurrency() / 2);
			break;
		}
	}

	void Engine::InitImgui()
	{
		m_UI.Initialize();
	}

	void Engine::InitWindow()
	{
		const std::string windowName = "TestWindow";
		m_Window.Initialize(windowName, 1280, 720);
	}

	void Engine::InitGraphics()
	{
		// Get required VK extensions
		m_EngineSettings.gfxSettings.requiredExtensions = m_Window.GetRequiredExtensions();
		m_Q->add(std::mem_fn(&Engine::Cmd_InitGraphics), std::make_shared<Window>(m_Window));
	}

	void Engine::CleanUpThreading()
	{
		if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
		{
			m_Q->add(std::mem_fn(&Engine::Cmd_ShutDown), std::make_shared<Window>(m_Window));
			m_SyncPoint->arrive_and_wait();
			m_Worker->Join();
			m_ThreadPool->wait_for_tasks();
			delete m_Worker;
			delete m_SyncPoint;
			delete m_ThreadPool;
		}
		delete m_Q;
	}

	void Engine::CleanUpWindow()
	{
		m_Window.Close();
	}

	void Engine::CleanUpGraphics()
	{
		m_Gfx.Destroy();
	}

	void Engine::CleanUpUI()
	{
		m_UI.Destroy();
	}

	void Engine::LoadDefaultStuff()
	{			
#if !BENCHMARK_MODE
		//AddDemoEntity(5000);
		//AddDemoEntity(kMaxDrawCount - 1);
#endif
	}

	void Engine::CreateCameras()
	{
		const auto camera = m_Entities.create();
		const auto previewCamera = m_Entities.create();
		const auto identity = glm::mat4x4(1.0f);

		static constexpr float defaultCameraYRotationRad = 0;
		const auto defaultCameraTransform = glm::rotate(glm::translate(identity, glm::vec3(0.0f, 0.0f, 15.0f)), defaultCameraYRotationRad, glm::vec3(0.0f, 1.0f, 0.0f));
		m_Entities.emplace<Comp::Transform>(camera, defaultCameraTransform);
		glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 5.0f, 1000.0f);
		m_Entities.emplace<Comp::Camera>(camera, proj, glm::mat4x4(), kCamOutColor, true, false, true);
#if BENCHMARK_MODE
		m_InitialCameraTransform = defaultCameraTransform;
#endif

		m_Entities.emplace<Comp::Transform>(previewCamera, glm::translate(defaultCameraTransform, glm::vec3(0.0f, 0.0f, 100.0f)));
		m_Entities.emplace<Comp::Camera>(previewCamera, proj, glm::mat4x4(), kCamOutColor, true, true, false);
	}

	void Engine::RenderCameras()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_RenderCameras), std::shared_ptr<void>());
	}

	void Engine::RenderImGUI()
	{
		// TODO:
		// Because dear imgui uses static globals I can't copy relevant data
		// So I need to update imgui right before launching render command
		// This brings the issue of latency for UI

		// TODO: because of the above problem need to signal this camera isn't dirty anymore
		// because currently it can only change in the gui
		const auto cameras = m_Entities.view<Comp::Camera>();
		for (auto ent : cameras)
		{
			auto& cam = cameras.get<Comp::Camera>(ent);
			cam.dirty = false;
		}

		m_UI.Update(*this, m_Entities);
		m_Q->add(std::mem_fn(&Engine::Cmd_RenderImGUI), std::shared_ptr<void>());
	}

	void Engine::UpdateRegistry()
	{
		UpdateCameras();
	}

	void Engine::UpdateCameras()
	{
		const auto cameras = m_Entities.view<Comp::Transform, Comp::Camera>();
		for (auto ent : cameras)
		{
			const auto& transform = cameras.get<Comp::Transform>(ent);
			auto& cam = cameras.get<Comp::Camera>(ent);

			// Vulkan's fixed-function steps expect to look down -Z, Y is "down" and RH
			static constexpr glm::vec3 front(0.0f, 0.0f, -1.0f);// look down -z
			static constexpr glm::vec3 up(0.0f, -1.0f, 0.0f);

			const auto quat = glm::toQuat(transform.transform);
			const auto newFront = glm::rotate(quat, front);
			const auto newUp = glm::rotate(quat, up);

			// update view matrix
			const auto pos = transform.GetPosition();
			cam.view = glm::lookAtRH(pos, pos + newFront, newUp);
		}
	}

	void Engine::Cull()
	{
#if BENCHMARK_MODE
		if (m_CollectBenchmarkData)
			m_CullTimer.start();
#endif
#if CULLING_ENABLED
		utils::Cull(m_Entities, m_VisibleDrawData, m_Gfx, *m_ThreadPool);
#endif

#if BENCHMARK_MODE
		if (m_CollectBenchmarkData)
			m_CullTimer.stop();
#endif
	}

	inline void GenerateIndirectDrawCommand(IGPUBuffer& dstBuffer, const Comp::MeshGeometry& meshData, uint32_t meshId, bool isMeshPipeline)
	{
#if CULLING_ENABLED
		IndirectDrawCmd cmd;
		cmd.meshDataIndex = meshId;
		dstBuffer.push_back(&cmd, sizeof(IndirectDrawCmd));
#else
		if (!isMeshPipeline)
		{
			VkDrawIndexedIndirectCommand cmd = {};
			cmd.indexCount = meshData.indices[0].GetCount();
			cmd.firstIndex = meshData.indices[0].GetOffset();
			cmd.instanceCount = 1;
			cmd.firstInstance = 0;
			cmd.vertexOffset = meshData.vertices.GetOffset();
			dstBuffer.push_back(&cmd, sizeof(VkDrawIndexedIndirectCommand));
			return;
		}
		ms_IndirectDrawCommand mcmd;
		mcmd.firstTask = 0;
		mcmd.taskCount = CONE_CULLING_ENABLED ? (meshData.meshlets[0].GetCount() + MESH_WGROUP - 1) / MESH_WGROUP : meshData.meshlets[0].GetCount();
		mcmd.meshTaskCount = meshData.meshlets[0].GetCount();
		mcmd.meshletBufferOffset = meshData.meshlets[0].GetOffset();
		dstBuffer.push_back(&mcmd, sizeof(ms_IndirectDrawCommand));
#endif
	}

	// This member function gets executed when both main and render thread arrive at the barrier.
	// Can be used to sync data.
	// Keep as fast as possible.
	// TODO: remove barrier to prevent whole engine stall
	// TODO: figure out why I'm copying this data around? Is there a way to prevent doubling down this data?
	// at least remove these dumb duplicate types like 'CameraData', just use Camera component
	void Engine::EngineThreadSyncFunc() noexcept
	{
		AUTO_TIMER("[ENGINE SYNC]: ");
		m_SyncTime.start();
		m_Window.UpdateDeltaTime();

		const auto renderMode = GetCurrentRenderMode();
		bool isMeshPipe = renderMode == kEngineRenderModeGPUDrivenMeshShading;

		switch (renderMode)
		{
		case kEngineRenderModeTraditional:
		{
#if CULLING_ENABLED
			auto& srcDrawData = m_VisibleDrawData;
#else 
			if (IsDrawDataDirty())
			{
				const auto transforms = m_Entities.view<Comp::Transform>();
				const auto group = m_Entities.group<Comp::ChildComponent, Comp::Mesh, Comp::Material>();
				const auto groupSize = group.size();
				m_VisibleDrawData.resize(0);

				for (const auto ent : group)
				{
					const auto& mesh = group.get<Comp::Mesh>(ent);
					const auto& parent = group.get<Comp::ChildComponent>(ent).parent;
					const auto& transform = transforms.get<Comp::Transform>(parent);

					DrawDataSingle dds;
					dds.Transform = transform.transform;
					dds.VertexBufferId = mesh.meshId;
					dds.LodIdx = 0;

					m_VisibleDrawData.push_back(dds);
				}
			}
			auto& srcDrawData = m_VisibleDrawData;
#endif

			auto& dstDrawData = m_Gfx.m_DrawData;
			dstDrawData.resize(0);

			// We already composed the draw data in Engine::Cull, can just copy it in
			dstDrawData.insert(dstDrawData.end(), srcDrawData.begin(), srcDrawData.end());
			break;
		}
		case kEngineRenderModeGPUDriven:
		case kEngineRenderModeGPUDrivenMeshShading:
		{
			AUTO_TIMER("[ENGINE SYNC - GPU-Driven part]: ");
			// This can stall because it may wait on timeline semaphore
			IGPUBuffer& drawCmdBuffer = m_Gfx.GetDrawCommandStagingBuffer();
			drawCmdBuffer.resize(0, 0);

			IGPUBuffer& drawDataBuffer = m_Gfx.GetDrawDataBuffer();
			drawDataBuffer.resize(0, 0);

			const auto renderableChildren = m_Entities.view<Comp::ChildComponent, Comp::Mesh, Comp::Material>();
			const auto transforms = m_Entities.view<Comp::Transform>();
			for (auto ent : renderableChildren)
			{
				const auto& mesh = renderableChildren.get<Comp::Mesh>(ent);
				const auto& parent = renderableChildren.get<Comp::ChildComponent>(ent).parent;
			    const auto& transform = transforms.get<Comp::Transform>(parent);

				const auto& meshData = m_Gfx.GetMeshData(mesh.meshId);
				if (IsDrawDataDirty())
				{
					// Needed for GPU-driven
					GenerateIndirectDrawCommand(drawCmdBuffer, meshData, mesh.meshId, isMeshPipe);
				}

				// Also update shader draw data. Also contains mesh id, that CPU-driven can use to generate draw commands
				ShaderDrawData sdd;
				sdd.transform = transform.transform;
				sdd.materialIndex = kDefaultMaterialIndex;
				sdd.vertexOffset = 0;
				// This seems to be quite slow, might be faster if I'd templatize the VulkanBuffer container
				drawDataBuffer.push_back(&sdd, sizeof(sdd));
			}
			if (IsDrawDataDirty())
			{
				// Mark delay so we do transfers in UpdateDraws and not in StartFrame
				m_Gfx.m_DelayTransferOperation = true;
				m_Q->add(std::mem_fn(&Engine::Cmd_UpdateDraws), std::shared_ptr<void>());
				m_DrawDataDirty = false;
			}
			break;
		}
		}

		const auto cameras = m_Entities.view<Comp::Transform, Comp::Camera>();
		for (auto ent : cameras)
		{
			const auto& transform = cameras.get<Comp::Transform>(ent);
			auto& cam = cameras.get<Comp::Camera>(ent);

			// Supporting only 1 camera and 1 "fake offset" camera
			static constexpr uint32_t cameraID = 0;
			if (cam.preview)
				m_Gfx.m_PreviewCamera = { cam.projection, cam.view, transform.transform, cam.camOutputType, cameraID, cam.dirty, cam.preview, cam.isRenderCamera };
			else
				m_Gfx.m_MainCamera = { cam.projection, cam.view, transform.transform, cam.camOutputType, cameraID, cam.dirty, cam.preview, cam.isRenderCamera };

			cam.dirty = false;
		}
		m_SyncTime.stop();

		auto& gfxTimer = m_Gfx.GetFrameTimings();
		auto& oldTimer = m_Gfx.GetOldFrameTimings();
		auto& gfxSyncTimer = m_Gfx.GetSyncTimings();
		auto& gfxOldSyncTimer = m_Gfx.GetOldSyncTimings();

		gfxTimer.totalFrameTime.stop();
		gfxTimer.waitTime.elapsed_time = gfxTimer.totalFrameTime.elapsed_time - gfxTimer.frameWorkTime.elapsed_time - m_SyncTime.elapsed_time;
		oldTimer = gfxTimer;
		gfxOldSyncTimer = gfxSyncTimer;

		gfxTimer.StartAll();
	}

}