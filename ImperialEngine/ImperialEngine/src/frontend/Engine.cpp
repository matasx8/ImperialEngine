#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Engine.h"
#include "Utils/GfxUtilities.h"
#include <barrier>
#include <extern/IMGUI/imgui.h>
#include "Components/Components.h"
#include <extern/GLM/ext/matrix_transform.hpp>
#include <extern/GLM/ext/matrix_clip_space.hpp>
#include <extern/GLM/gtx/quaternion.hpp>

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_DrawDataDirty(false), m_Q(nullptr), m_Worker(nullptr), m_SyncPoint(nullptr), m_EngineSettings(), m_Window(), m_UI(), m_Gfx(), m_CulledDrawData()/*, m_BVs()*/, m_AssetImporter(*this)
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
		// TODO: this is needed as a workaround when launching the actual .exe since it will have different current path it won't know how to access Assets
		//std::filesystem::current_path("D:\\source\\ImperialEngine\\ImperialEngine\\ImperialEngine");
		std::filesystem::current_path("C:\\Users\\mtunk\\source\\repos\\ImperialEngine\\ImperialEngine\\ImperialEngine");
		m_EngineSettings = settings;
		InitThreading(m_EngineSettings.threadingMode);
		InitImgui();
		InitWindow();
		InitGraphics();
		InitAssetImporter();

		// unfortunately we must wait until imgui is initialized on the backend
		m_SyncPoint->arrive_and_wait();
		return true;
	}

	void Engine::LoadScene()
	{
		m_AssetImporter.LoadScene("Scene/");
		m_AssetImporter.LoadMaterials("Shaders/spir-v");
		m_AssetImporter.LoadComputeProgams("Shaders/spir-v");
		LoadDefaultStuff();
		MarkDrawDataDirty();
	}

	void Engine::StartFrame()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_StartFrame), std::shared_ptr<void>());
	}

	void Engine::Update()
	{
		// not sure if this should be here
		m_Window.UpdateImGUI();
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
		RenderImGUI();
	}

	void Engine::EndFrame()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_EndFrame), std::shared_ptr<void>());
	}

	void Engine::SyncRenderThread()
	{
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

	bool Engine::IsCurrentRenderMode(EngineRenderMode mode) const
	{
		return m_EngineSettings.gfxSettings.renderMode == mode;
	}

	EngineRenderMode Engine::GetCurrentRenderMode() const
	{
		return m_EngineSettings.gfxSettings.renderMode;
	}

	void Engine::SwitchRenderingMode(EngineRenderMode newRenderMode)
	{
		m_EngineSettings.gfxSettings.renderMode = newRenderMode;
		m_Q->add(std::mem_fn(&Engine::Cmd_ChangeRenderMode), std::make_shared<EngineRenderMode>(newRenderMode));
		MarkDrawDataDirty();
	}

	void Engine::AddDemoEntity(uint32_t count)
	{
		if (count)
			MarkDrawDataDirty();

		static constexpr uint32_t numMeshes = 4u;

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
		return m_Window.ShouldClose();
	}

	void Engine::ShutDown()
	{
		CleanUpThreading();
		CleanUpWindow();
		CleanUpUI();
		CleanUpGraphics();
	}

	void Engine::InitThreading(EngineThreadingMode mode)
	{
		BarrierFunctionObject func(*this);
		switch (mode)
		{
		case kEngineSingleThreaded:
			// single-threaded so we only need the single-threaded version of the queue that executes task on add.
			m_Q = new prl::WorkQ_ST<Engine>(*this);

			m_SyncPoint = new std::barrier(1, func);
			break;
		case kEngineMultiThreaded:
			constexpr int numThreads = 2;
			m_Q = new prl::WorkQ<Engine>();
			m_Worker = new prl::ConsumerThread<Engine>(*this, *m_Q);
			m_SyncPoint = new std::barrier(numThreads, func);
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

	void Engine::InitAssetImporter()
	{
		m_AssetImporter.Initialize();
	}

	void Engine::CleanUpThreading()
	{
		if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
		{
			m_Worker->End();				// signal to stop working
			m_SyncPoint->arrive_and_wait();
			m_Worker->Join();
			delete m_Worker;
			delete m_SyncPoint;
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
		const auto camera = m_Entities.create();
		const auto previewCamera = m_Entities.create();
		const auto identity = glm::mat4x4(1.0f);

		static constexpr float defaultCameraYRotationRad = 0;
		const auto defaultCameraTransform = glm::rotate(glm::translate(identity, glm::vec3(0.0f, 0.0f, 15.0f)), defaultCameraYRotationRad, glm::vec3(0.0f, 1.0f, 0.0f));
		m_Entities.emplace<Comp::Transform>(camera, defaultCameraTransform);
		glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 5.0f, 1000.0f);
		m_Entities.emplace<Comp::Camera>(camera, proj, glm::mat4x4(), kCamOutColor, true, false, true);

		m_Entities.emplace<Comp::Transform>(previewCamera, glm::translate(defaultCameraTransform, glm::vec3(0.0f, 0.0f, 100.0f)));
		m_Entities.emplace<Comp::Camera>(previewCamera, proj, glm::mat4x4(), kCamOutColor, true, true, false);

		AddDemoEntity(5000);
		//AddDemoEntity(kMaxDrawCount - 1);
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
		// TODO mesh: reenable this
		//assert(IsCurrentRenderMode(kEngineRenderModeTraditional));
		m_CulledDrawData.resize(0);

		const auto cameras = m_Entities.view<Comp::Transform, Comp::Camera>();
		const auto& cam = cameras.get<Comp::Camera>(cameras.back());
		const glm::mat4x4 VP = cam.projection * cam.view;

		const auto frustumPlanes = utils::FindViewFrustumPlanes(VP);

		const auto renderableChildren = m_Entities.view<Comp::ChildComponent, Comp::Mesh, Comp::Material>();
		const auto transforms = m_Entities.view<Comp::Transform>();
		uint32_t totalMeshes = 0;
		for (auto ent : renderableChildren)
		{
			totalMeshes++;
			const auto& mesh = renderableChildren.get<Comp::Mesh>(ent);
			const auto& parent = renderableChildren.get<Comp::ChildComponent>(ent).parent;
			const auto& transform = transforms.get<Comp::Transform>(parent);
			const auto& BV = m_Gfx.m_BVs.at(mesh.meshId);

			glm::vec4 mCenter = glm::vec4(BV.center, 1.0f);
			glm::vec4 wCenter = transform.transform * glm::vec4(BV.center, 1.0f);

			bool isVisible = true;
			for (auto i = 0; i < 6; i++)
			{
				float dotProd = glm::dot(frustumPlanes[i], wCenter);
				if (dotProd < -BV.diameter)
				{
					isVisible = false;
					break;
				}
			}
			
			if (isVisible)
			{
				auto lodIdx = utils::ChooseMeshLODByNearPlaneDistance(transform.transform, BV, VP);

				m_CulledDrawData.emplace_back(transform.transform, mesh.meshId, lodIdx);
			}
		}

		//printf("[CPU CULL] Total Renderable Meshes: %u; Renderable Meshes after culling: %llu\n", totalMeshes, m_CulledDrawData.size());
	}

	inline void GenerateIndirectDrawCommand(IGPUBuffer& dstBuffer, const Comp::MeshGeometry& meshData, uint32_t meshId)
	{
		IndirectDrawCmd cmd;
		cmd.meshDataIndex = meshId;
		dstBuffer.push_back(&cmd, sizeof(IndirectDrawCmd));
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

		switch (renderMode)
		{
		case kEngineRenderModeTraditional:
		{
			auto& srcDrawData = m_CulledDrawData;
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
			drawCmdBuffer.resize(0);

			IGPUBuffer& drawDataBuffer = m_Gfx.GetDrawDataBuffer();
			drawDataBuffer.resize(0);

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
					GenerateIndirectDrawCommand(drawCmdBuffer, meshData, mesh.meshId);
				}

				// Also update shader draw data. Also contains mesh id, that CPU-driven can use to generate draw commands
				ShaderDrawData sdd;
				sdd.transform = transform.transform;
				sdd.materialIndex = kDefaultMaterialIndex;
				sdd.vertexOffset = 0;
				sdd.meshletOffset = meshData.meshletOffset;
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
				m_Gfx.m_PreviewCamera = { cam.projection, cam.view, cam.camOutputType, cameraID, cam.dirty, cam.preview, cam.isRenderCamera };
			else
				m_Gfx.m_MainCamera = { cam.projection, cam.view, cam.camOutputType, cameraID, cam.dirty, cam.preview, cam.isRenderCamera };

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