#pragma once
#include "frontend/Engine.h"

namespace imp
{
	void Engine::Cmd_InitGraphics(std::shared_ptr<void> rsc)
	{
		auto re = (Window*)rsc.get();
		m_Gfx.Initialize(m_EngineSettings.gfxSettings, re);
		// after initing graphics we can now wait for first update
		m_SyncPoint->arrive_and_wait();
	}

	void Engine::Cmd_StartFrame(std::shared_ptr<void> rsc)
	{
		m_Gfx.StartFrame();
	}

	void Engine::Cmd_RenderCameras(std::shared_ptr<void> rsc)
	{
		m_Gfx.RenderCameras();
	}

	void Engine::Cmd_EndFrame(std::shared_ptr<void> rsc)
	{
		m_Gfx.EndFrame();
	}
	void Engine::Cmd_SyncRenderThread(std::shared_ptr<void> rsc)
	{
		m_SyncPoint->arrive_and_wait();
	}
	void Engine::Cmd_RenderImGUI(std::shared_ptr<void> rsc)
	{
		m_Gfx.RenderImGUI();
	}
	void Engine::Cmd_UploadMeshes(std::shared_ptr<void> rsc)
	{
		auto re = (std::vector<imp::MeshCreationRequest>*)rsc.get();
		// we have place where to add index and vertex components
		// we know vert and idx data

		m_Gfx.CreateAndUploadMeshes(*re);
	}

	void Engine::Cmd_UploadMaterials(std::shared_ptr<void> rsc)
	{
		auto re = (std::vector<imp::MaterialCreationRequest>*)rsc.get();
		m_Gfx.CreateAndUploadMaterials(*re);
	}

	void Engine::Cmd_UploadComputePrograms(std::shared_ptr<void> rsc)
	{
		auto re = (std::vector<imp::ComputeProgramCreationRequest>*)rsc.get();
		m_Gfx.CreateComputePrograms(*re);
	}

	void Engine::Cmd_ChangeRenderMode(std::shared_ptr<void> rsc)
	{
		// Changing settings should only happen at start of the frame.
		// Later move this command to someplace else on main thread
		auto* re = (EngineRenderMode*)rsc.get();
		m_Gfx.GetGraphicsSettings().renderMode = *re;
	}

	void Engine::Cmd_UpdateDraws(std::shared_ptr<void> rsc)
	{
		m_Gfx.UpdateDrawCommands();
	}

	void Engine::Cmd_ShutDown(std::shared_ptr<void> rsc)
	{
		m_Worker->End();
	}

#if BENCHMARK_MODE
	void Engine::Cmd_StartBenchmark(std::shared_ptr<void> rsc)
	{
		m_Gfx.StartBenchmark();
	}
#endif
}