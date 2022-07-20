#pragma once
#include "Utils/NonCopyable.h"
#include "extern/ENTT/entt.hpp"
#include "backend/parallel/WorkQ_ST.h"
#include "backend/parallel/ConsumerThread.h"
#include "frontend/EngineSettings.h"
#include "frontend/Window.h"

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

		bool ShouldClose() const;
		void ShutDown();
	private:

		void InitThreading(EngineThreadingMode mode);
		void InitWindow();
		void CleanUpThreading();
		void CleanUpWindow();

		// entity stuff
		entt::registry m_Entities;

		// parallel stuff
		prl::WorkQ<Engine>* m_Q;
		prl::ConsumerThread<Engine>* m_Worker;

		// window stuff
		Window m_Window;

		EngineSettings m_EngineSettings;
	};
}

