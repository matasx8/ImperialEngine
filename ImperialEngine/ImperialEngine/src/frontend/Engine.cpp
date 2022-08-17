#include "Engine.h"
#include "backend/EngineCommandResources.h"
#include <barrier>
#include <extern/IMGUI/imgui.h>
#include "Components/Components.h"

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_Q(nullptr), m_Worker(nullptr), m_SyncPoint(nullptr), m_EngineSettings(), m_Window(), m_UI(), m_Gfx(), m_AssetImporter(*this)
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
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
	}

	void Engine::StartFrame()
	{
	}

	void Engine::Update()
	{
		// not sure if this should be here
		m_Window.UpdateImGUI();
		m_Window.Update();
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
		if (m_EngineSettings.threadingMode == kEngineMultiThreaded)
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
		m_Window.Initialize(windowName, 1280 , 720 );
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

	void Engine::RenderCameras()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_RenderCameras), std::shared_ptr<void>());
	}

	void Engine::RenderImGUI()
	{
		// Because dear imgui uses static globals I can't copy relevant data
		// TODO: find out how to better paralelize imgui
		m_UI.Update();
		m_Q->add(std::mem_fn(&Engine::Cmd_RenderImGUI), std::shared_ptr<void>());
	}


	// This member function gets executed when both main and render thread arrive at the barrier.
	// Can be used to sync data.
	// Keep as fast as possible.
	void Engine::EngineThreadSyncFunc() noexcept
	{
		m_Window.UpdateDeltaTime();
		m_Gfx.m_DrawData.clear();

		const auto meshes = m_Entities.view<Comp::Mesh>();
		const auto transforms = m_Entities.view<Comp::Transform>();
		for (auto ent : meshes)
		{
			const auto& mesh = meshes.get<Comp::Mesh>(ent);
			const auto& transform = transforms.get<Comp::Transform>(mesh.e);

			m_Gfx.m_DrawData.emplace_back(transform.transform, static_cast<uint32_t>(mesh.e));
		}
	}

}