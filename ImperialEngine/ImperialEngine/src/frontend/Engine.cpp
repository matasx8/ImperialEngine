#include "Engine.h"

namespace imp
{
	Engine::Engine()
		: m_Entities(), m_Q(nullptr), m_Worker(nullptr), m_EngineSettings()
	{
	}

	bool Engine::Initialize(EngineSettings settings)
	{
		InitThreading(settings.threadingMode);

		m_EngineSettings = settings;
		return true;
	}

	void Engine::Update()
	{
	}

	void Engine::Render()
	{
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

}