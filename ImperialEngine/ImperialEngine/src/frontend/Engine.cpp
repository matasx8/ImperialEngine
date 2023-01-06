#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Engine.h"
#include "backend/EngineCommandResources.h"
#include <barrier>
#include <extern/IMGUI/imgui.h>
#include "Components/Components.h"
#include <extern/GLM/ext/matrix_transform.hpp>
#include <extern/GLM/ext/matrix_clip_space.hpp>
#include <extern/GLM/gtx/quaternion.hpp>
#include <extern/IPROF/iprof.hpp>

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_Q(nullptr), m_Worker(nullptr), m_SyncPoint(nullptr), m_EngineSettings(), m_Window(), m_UI(), m_Gfx(), m_AssetImporter(*this)
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
		SyncRenderThread();	// and then insert another barrier for first frame
		return true;
	}

	void Engine::LoadScene()
	{
		m_AssetImporter.LoadScene("Scene/");
		m_AssetImporter.LoadMaterials("Shaders/spir-v");
		LoadDefaultStuff();
	}

	void Engine::StartFrame()
	{
		IPROF_FUNC;
		m_Q->add(std::mem_fn(&Engine::Cmd_StartFrame), std::shared_ptr<void>());
	}

	void Engine::Update()
	{
		IPROF_FUNC;
		// not sure if this should be here
		m_Window.UpdateImGUI();
		m_Window.Update();
		UpdateRegistry();

	}

	void Engine::Render()
	{
		IPROF_FUNC;
		RenderCameras();
		RenderImGUI();
	}

	void Engine::EndFrame()
	{
		IPROF_FUNC;
		m_Q->add(std::mem_fn(&Engine::Cmd_EndFrame), std::shared_ptr<void>());
	}

	void Engine::SyncRenderThread()
	{
		if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
			m_Q->add(std::mem_fn(&Engine::Cmd_SyncRenderThread), std::shared_ptr<void>());
	}

	void Engine::SyncGameThread()
	{
		InternalProfiler::aggregateEntries();
		InternalProfiler::addThisThreadEntriesToAllThreadStats();
		IPROF_FUNC;
			m_SyncPoint->arrive_and_wait();
	}

	void Engine::AddMonkey(uint32_t monkeyCount)
	{
		static int idx = 0;
		static glm::vec3 offset = glm::vec3(0.0f, 0.0f, 2.0f);
		for (auto i = 0; i < monkeyCount; i++)
		{
			switch (idx)
			{
			case 0:
				offset += glm::vec3(2.0f, 0.0f, 0.0f);
				break;
			case 1:
				offset += glm::vec3(0.0f, 2.0f, 0.0f);
				break;
			case 2:
				offset += glm::vec3(0.0f, 0.0f, 2.0f);
				break;
			}
			idx++;
			idx %= 3;
			auto& reg = m_Entities;
			const auto monkey = reg.create();
			const auto identityMat = glm::mat4x4(1.0f);
			const auto newMatrix = glm::translate(identityMat, offset);
			reg.emplace<Comp::Transform>(monkey, newMatrix);
			
			const auto child = reg.create();
			reg.emplace<Comp::ChildComponent>(child, monkey);
			reg.emplace<Comp::Mesh>(child, 0u);
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

	void Engine::CleanUpAssetImporter()
	{
		m_AssetImporter.Destroy();
	}

	static glm::mat4x4 makeProj(auto aspect)
	{
		const auto h = 1.0 / glm::tan((glm::radians(45.0f) * 0.5));
		const auto w = h / aspect;
		const auto znear = 0.1f;
		const auto zfar = 100.0f;
		const auto a = -znear / (zfar - znear);
		const auto b = (znear * zfar) / (zfar - znear);
		return glm::mat4x4(
			glm::vec4(w, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, -h, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, a, 1.0f),
			glm::vec4(0.0f, 0.0f, b, 0.0f));
	}

	void Engine::LoadDefaultStuff()
	{			
		const auto camera = m_Entities.create();
		const auto identity = glm::mat4x4(1.0f);
		const auto defaultCameraTransform = glm::translate(identity, glm::vec3(-15.0f, 0.0f, 0.0f));
		m_Entities.emplace<Comp::Transform>(camera, defaultCameraTransform);
		glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 1.0f, 100.0f);
		m_Entities.emplace<Comp::Camera>(camera, proj, glm::mat4x4(), kCamOutColor, true);
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

			static constexpr glm::vec3 front(0.0f, 0.0f, -1.0f);// look down -z
			static constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);

			const auto quat = glm::toQuat(transform.transform);
			const auto newFront = glm::rotate(quat, front);
			const auto newUp = glm::rotate(quat, up);

			// update view matrix
			const auto pos = transform.GetPosition();
			cam.view = glm::lookAtLH(pos, pos + newFront, newUp);

			static constexpr auto intermTransform = glm::mat4x4(
				glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
				glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
				glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			cam.view = intermTransform * cam.view;
		}
	}

	// This member function gets executed when both main and render thread arrive at the barrier.
	// Can be used to sync data.
	// Keep as fast as possible.
	// TODO: remove barrier to prevent whole engine stall
	// TODO: figure out why I'm copying this data around? Is there a way to prevent doubling down this data?
	// at least remove these dumb duplicate types like 'CameraData', just use Camera component
	void Engine::EngineThreadSyncFunc() noexcept
	{
		IPROF("Engine::EngineThreadSync");
		m_Window.UpdateDeltaTime();
		m_Gfx.m_DrawData.clear();
		m_Gfx.m_CameraData.clear();

		const auto renderableChildren = m_Entities.view<Comp::ChildComponent, Comp::Mesh, Comp::Material>();
		const auto transforms = m_Entities.view<Comp::Transform>();
		for (auto ent : renderableChildren)
		{
			const auto& mesh = renderableChildren.get<Comp::Mesh>(ent);
			//const Comp::Material& material = renderableChildren.get<Comp::Material>(ent);
			const auto& parent = renderableChildren.get<Comp::ChildComponent>(ent).parent;
			const auto& transform = transforms.get<Comp::Transform>(parent);

			m_Gfx.m_DrawData.emplace_back(transform.transform, mesh.meshId);
		}

		const auto cameras = m_Entities.view<Comp::Transform, Comp::Camera>();
		for (auto ent : cameras)
		{
			const auto& transform = cameras.get<Comp::Transform>(ent);
			const auto& cam = cameras.get<Comp::Camera>(ent);

			// should try to save bandwidth and do most calculation on main thread then send the data to render thread
			static constexpr uint32_t cameraID = 0; // TODO: add this to camera comp
			m_Gfx.m_CameraData.emplace_back(cam.projection, cam.view, cam.camOutputType, cameraID, cam.dirty);
		}
	}

}