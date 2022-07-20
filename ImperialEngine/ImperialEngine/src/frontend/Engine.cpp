#include "Engine.h"

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_Q(nullptr), m_Worker(nullptr), m_EngineSettings(), m_Window()
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
		InitThreading(settings.threadingMode);
		InitWindow();

		m_EngineSettings = settings;
		return true;
	}

	void Engine::Update()
	{
		m_Window.Update();

	}

	void Engine::Render()
	{
	}

	bool Engine::ShouldClose() const
	{
		return m_Window.ShouldClose();
	}

	void Engine::ShutDown()
	{
		CleanUpThreading();
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

}