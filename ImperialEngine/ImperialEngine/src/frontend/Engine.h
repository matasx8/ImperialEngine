#pragma once
#include "Utils/NonCopyable.h"
#include "extern/ENTT/entt.hpp"
#include "backend/graphics/Graphics.h"
#include "backend/parallel/WorkQ_ST.h"
#include "backend/parallel/ConsumerThread.h"
#include "frontend/EngineSettings.h"
#include "frontend/AssetImporter.h"
#include "frontend/Window.h"
#include "frontend/UI.h"
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
		void LoadScene();

		void StartFrame();
		void Update();
		void Render();
		void EndFrame();
		void SyncRenderThread();
		void SyncGameThread();

		bool ShouldClose() const;
		void ShutDown();
	private:

		void InitThreading(EngineThreadingMode mode);
		void InitImgui();
		void InitWindow();
		void InitGraphics();
		void InitAssetImporter();
		void CleanUpThreading();
		void CleanUpWindow();
		void CleanUpGraphics();
		void CleanUpUI();
		void CleanUpAssetImporter();

		void RenderCameras();
		void RenderImGUI();

		void EngineThreadSyncFunc()  noexcept;


		// entity stuff
		entt::registry m_Entities;

		// parallel stuff
		prl::WorkQ<Engine>* m_Q;
		prl::ConsumerThread<Engine>* m_Worker;
		struct BarrierFunctionObject
		{
			Engine& engine;
			BarrierFunctionObject(Engine& eng) : engine(eng) {};
			void operator()() noexcept { engine.EngineThreadSyncFunc(); }
		};
	    std::barrier<BarrierFunctionObject>* m_SyncPoint;	// TODO: use polymorphism for when Single thread mode this wont do anything

		// window stuff
		Window m_Window;
		UI m_UI;

		// graphics stuff
		Graphics m_Gfx;

		// asset stuff
		friend class AssetImporter;
		AssetImporter m_AssetImporter;

		EngineSettings m_EngineSettings;

		// can I put this in a different namespace and drop the prefix?
		void Cmd_InitGraphics(std::shared_ptr<void> rsc);
		void Cmd_RenderCameras(std::shared_ptr<void> rsc);
		void Cmd_EndFrame(std::shared_ptr<void> rsc);
		void Cmd_SyncRenderThread(std::shared_ptr<void> rsc);
		void Cmd_RenderImGUI(std::shared_ptr<void> rsc);
		void Cmd_UploadMeshes(std::shared_ptr<void> rsc);
		void Cmd_UploadMaterials(std::shared_ptr<void> rsc);
	};
}

