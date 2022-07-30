#pragma once
#include "Utils/NonCopyable.h"
#include "extern/ENTT/entt.hpp"
#include "backend/graphics/Graphics.h"
#include "backend/parallel/WorkQ_ST.h"
#include "backend/parallel/ConsumerThread.h"
#include "frontend/EngineSettings.h"
#include "frontend/Window.h"
#include <barrier>

// No Vulkan stuff here
// use registry from EnTT for commands and their memory?
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

namespace imp
{
	class Engine : NonCopyable
	{
	public:
		Engine();
		bool Initialize(EngineSettings settings);

		void StartFrame();
		void Update();
		void Render();
		void EndFrame();

		bool ShouldClose() const;
		void ShutDown();
	private:

		void InitThreading(EngineThreadingMode mode);
		void InitWindow();
		void InitGraphics();
		void CleanUpThreading();
		void CleanUpWindow();
		void CleanUpGraphics();

		void RenderCameras();

		void EngineThreadSyncFunc()  noexcept;

		// entity stuff
		entt::registry m_Entities;

		// parallel stuff
		prl::WorkQ<Engine>* m_Q;
		prl::ConsumerThread<Engine>* m_Worker;
	    std::barrier<void (*)() noexcept>* m_SyncPoint;
		//std::barrier<void (Engine::*)() noexcept>>* m_SyncPoint;

		// window stuff
		Window m_Window;

		// graphics stuff
		Graphics m_Gfx;

		EngineSettings m_EngineSettings;

		void Cmd_InitGraphics(std::shared_ptr<void> rsc);
		void Cmd_RenderCameras(std::shared_ptr<void> rsc);
		void Cmd_EndFrame(std::shared_ptr<void> rsc);
	};
}

