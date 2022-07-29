#include "Engine.h"
#include "backend/EngineCommandResources.h"
#include <barrier>

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_Q(nullptr), m_Worker(nullptr), m_EngineSettings(), m_Window(), m_Gfx()
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
		m_EngineSettings = settings;
		InitThreading(m_EngineSettings.threadingMode);
		InitWindow();
		InitGraphics();

		return true;
	}

	void Engine::StartFrame()
	{
		// wait for gfx thread to stop
		// copy relevant memory to gfx thread visible memory
		// carry on
		// use barriers for that? Would need two but that looks quite simple to achieve
	}

	void Engine::Update()
	{
		m_Window.Update();

	}

	void Engine::Render()
	{
		RenderCameras();
	}

	void Engine::EndFrame()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_EndFrame), std::shared_ptr<void>());
	}

	bool Engine::ShouldClose() const
	{
		return m_Window.ShouldClose();
	}

	void Engine::ShutDown()
	{
		CleanUpThreading();
		CleanUpWindow();
		CleanUpGraphics();
	}

	void Engine::InitThreading(EngineThreadingMode mode)
	{
		switch (mode)
		{
		case kEngineSingleThreaded:
			// single-threaded so we only need the single-threaded version of the queue that executes task on add.
			m_Q = new prl::WorkQ_ST<Engine>(*this);
			break;
		case kEngineMultiThreaded:
			m_Q = new prl::WorkQ<Engine>();
			m_Worker = new prl::ConsumerThread<Engine>(*this, *m_Q);
			break;
		}
	}

	void Engine::InitWindow()
	{
		const std::string windowName = "TestWindow";
		m_Window.Initialize(windowName, 800, 600);
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
			m_Worker->End();
			m_Worker->Join();
			delete m_Worker;
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

	void Engine::RenderCameras()
	{
		m_Q->add(std::mem_fn(&Engine::Cmd_RenderCameras), std::shared_ptr<void>());
	}

}