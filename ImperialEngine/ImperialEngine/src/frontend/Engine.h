#pragma once
#include "Utils/NonCopyable.h"
#include "extern/ENTT/entt.hpp"
#include "backend/parallel/WorkQ_ST.h"
#include "backend/parallel/ConsumerThread.h"
#include "frontend/EngineSettings.h"

// No Vulkan stuff here
// use registry from EnTT for commands and their memory?
// use VULKAN_SDK path to include libraries

namespace imp
{
	class Engine : NonCopyable
	{
	public:
		Engine();
		bool Initialize(EngineSettings settings);

		void Update();
		void Render();

		void ShutDown();
	private:

		void InitThreading(EngineThreadingMode mode);
		void CleanUpThreading();

		// entity stuff
		entt::registry m_Entities;

		// parallel stuff
		prl::WorkQ<Engine>* m_Q;
		prl::ConsumerThread<Engine>* m_Worker;

		EngineSettings m_EngineSettings;
	};
}

