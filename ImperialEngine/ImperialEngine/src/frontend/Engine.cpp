#include "Engine.h"
#include "backend/EngineCommandResources.h"
#include <barrier>
#include <extern/IMGUI/imgui.h>
#include "Components/Components.h"
#include <extern/GLM/ext/matrix_transform.hpp>
#include <extern/GLM/ext/matrix_clip_space.hpp>
#include <extern/GLM/gtx/quaternion.hpp>

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
	}

	void Engine::Update()
	{
		// not sure if this should be here
		m_Window.UpdateImGUI();
		m_Window.Update();
		UpdateRegistry();

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
		m_Q->add(std::mem_fn(&Engine::Cmd_SyncRenderThread), std::shared_ptr<void>());
	}

	void Engine::SyncGameThread()
	{
		//if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
			m_SyncPoint->arrive_and_wait();
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

	void Engine::LoadDefaultStuff()
	{			
		// create main entity, that the renderable entities will point to
		//const entt::entity mainEntity = m_Engine.m_Entities.create();
		//auto& reg = m_Engine.m_Entities;
		//reg.emplace<Comp::Transform>(mainEntity, glm::mat4x4(1.0f));
		// camera
		const auto camera = m_Entities.create();
		const auto identity = glm::mat4x4(1.0f);
		const auto defaultCameraTransform = glm::translate(identity, glm::vec3(-15.0f, 0.0f, 0.0f));
		m_Entities.emplace<Comp::Transform>(camera, defaultCameraTransform);
		glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 0.1f, 10000.0f);
		const auto emptyvec = glm::vec3();
		m_Entities.emplace<Comp::Camera>(camera, proj);
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
		m_UI.Update(m_Entities);
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

			// update front, right up
			// !HERE rotate this using transform..
			static constexpr glm::vec3 front(1.0f, 0.0f, 0.0f);
			static constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);

			const auto quat = glm::toQuat(transform.transform);
			const auto newFront = glm::rotate(quat, front);
			const auto newUp = glm::rotate(quat, up);

			// update view matrix
			const auto pos = transform.GetPosition();
			cam.view = glm::lookAt(pos, pos + newFront, newUp);
		}
	}

	// This member function gets executed when both main and render thread arrive at the barrier.
	// Can be used to sync data.
	// Keep as fast as possible.
	void Engine::EngineThreadSyncFunc() noexcept
	{
		m_Window.UpdateDeltaTime();
		m_Gfx.m_DrawData.clear();
		m_Gfx.m_CameraData.clear();

		const auto meshes = m_Entities.view<Comp::Mesh>();
		const auto transforms = m_Entities.view<Comp::Transform>();
		for (auto ent : meshes)
		{
			const auto& mesh = meshes.get<Comp::Mesh>(ent);
			const auto& transform = transforms.get<Comp::Transform>(mesh.e);

			m_Gfx.m_DrawData.emplace_back(transform.transform, static_cast<uint32_t>(ent));
		}

		const auto cameras = m_Entities.view<Comp::Transform, Comp::Camera>();
		for (auto ent : cameras)
		{
			const auto& transform = cameras.get<Comp::Transform>(ent);
			const auto& cam = cameras.get<Comp::Camera>(ent);

			// should try to save bandwidth and do most calculation on main thread then send the data to render thread
			m_Gfx.m_CameraData.emplace_back(cam.projection, cam.view);
		}
	}

}