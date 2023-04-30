#pragma once
#include "Utils/FrameTimeTable.h"
#include "Utils/NonCopyable.h"
#include "Utils/SimpleTimer.h"
#include "extern/ENTT/entt.hpp"
#include "backend/graphics/Graphics.h"
#include "backend/parallel/WorkQ_ST.h"
#include "backend/parallel/ConsumerThread.h"
#include "frontend/AssetImporter.h"
#include "frontend/Window.h"
#include "frontend/UI.h"
#include <barrier>

// No Vulkan stuff here

namespace BS { class thread_pool; }

namespace imp
{
	class Engine : NonCopyable
	{
	public:
		Engine();
		bool Initialize(EngineSettings settings);
		void LoadScenes(const std::vector<std::string>& scenes);
		void LoadAssets();

		void DistributeEntities(const std::string& distribution, const std::string& entityCount);

		void StartFrame();
		void Update();
		void Render();
		void EndFrame();
		void SyncRenderThread();
		void SyncGameThread();

#if BENCHMARK_MODE
		void StartBenchmark();
		void StopBenchmark();
		const std::array<FrameTimeTable, kEngineRenderModeCount>& GetMainBenchmarkTable() const;
		const std::array<FrameTimeTable, kEngineRenderModeCount>& GetRenderBenchmarkTable() const;
#endif

		entt::registry& GetEntityRegistry();

		bool IsCurrentRenderMode(EngineRenderMode mode) const;
		EngineRenderMode GetCurrentRenderMode() const;
		bool IsRenderingModeSupported(EngineRenderMode mode) const;

		// Will affect the next frame
		void SwitchRenderingMode(EngineRenderMode newRenderMode);

		// temporary
		void AddDemoEntity(uint32_t count);

		bool ShouldClose() const;
		void ShutDown();
	private:

		void InitThreading(EngineThreadingMode mode);
		void InitImgui();
		void InitWindow();
		void InitGraphics();
		void CleanUpThreading();
		void CleanUpWindow();
		void CleanUpGraphics();
		void CleanUpUI();

		void LoadDefaultStuff();
		void CreateCameras();

		// Signal that number of draws have changed
		void MarkDrawDataDirty() { m_DrawDataDirty = true; }
		bool IsDrawDataDirty() { return m_DrawDataDirty; }

		void RenderCameras();
		void RenderImGUI();

		// update systems
		void UpdateRegistry();
		void UpdateCameras();

		void Cull();

		void EngineThreadSyncFunc()  noexcept;

		// entity stuff
		entt::registry m_Entities;

		bool m_DrawDataDirty;

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

		BS::thread_pool* m_ThreadPool;

		// graphics stuff
		Graphics m_Gfx;
		// Used as a "staging" buffer for CPU VF culling
		std::vector<DrawDataSingle> m_VisibleDrawData;

#if BENCHMARK_MODE
		glm::mat4x4 m_InitialCameraTransform;
		std::array<FrameTimeTable, kEngineRenderModeCount> m_FrameTimeTables;
#else
		CircularFrameTimeRowContainer m_FrameStats;
#endif
		SimpleTimer m_FrameTimer;
		SimpleTimer m_CullTimer;
		SimpleTimer m_FullFrameTimer;
		double m_LastFrameTime;
#if BENCHMARK_MODE
		bool m_BenchmarkDone;
		bool m_CollectBenchmarkData;
#endif

		// asset stuff
		friend class AssetImporter;
		AssetImporter m_AssetImporter;

		EngineSettings m_EngineSettings;

		// TODO prettier: put this into a namespace
		void Cmd_InitGraphics(std::shared_ptr<void> rsc);
		void Cmd_StartFrame(std::shared_ptr<void> rsc);
		void Cmd_RenderCameras(std::shared_ptr<void> rsc);
		void Cmd_EndFrame(std::shared_ptr<void> rsc);
		void Cmd_SyncRenderThread(std::shared_ptr<void> rsc);
		void Cmd_RenderImGUI(std::shared_ptr<void> rsc);
		void Cmd_UploadMeshes(std::shared_ptr<void> rsc);
		void Cmd_UploadMaterials(std::shared_ptr<void> rsc);
		void Cmd_UploadComputePrograms(std::shared_ptr<void> rsc);
		void Cmd_ChangeRenderMode(std::shared_ptr<void> rsc);
		void Cmd_UpdateDraws(std::shared_ptr<void> rsc);
		void Cmd_ShutDown(std::shared_ptr<void> rsc);

#if BENCHMARK_MODE
		void Cmd_StartBenchmark(std::shared_ptr<void> rsc);
		void Cmd_StopBenchmark(std::shared_ptr<void> rsc);
#endif
	};
}

