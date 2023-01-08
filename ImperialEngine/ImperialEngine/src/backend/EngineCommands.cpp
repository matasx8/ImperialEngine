#include "frontend/Engine.h"
#include <IPROF/iprof.hpp>

namespace imp
{
	void Engine::Cmd_InitGraphics(std::shared_ptr<void> rsc)
	{
		auto* re = (Window*)rsc.get();
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
		m_Gfx.Arrive(m_SyncPoint);
		m_Gfx.EndFrame();
	}
	void Engine::Cmd_SyncRenderThread(std::shared_ptr<void> rsc)
	{
		InternalProfiler::aggregateEntries();
		InternalProfiler::addThisThreadEntriesToAllThreadStats();
		IPROF_FUNC;
		m_Gfx.Wait(m_SyncPoint);
		//m_SyncPoint->arrive_and_wait();
	}
	void Engine::Cmd_RenderImGUI(std::shared_ptr<void> rsc)
	{
		m_Gfx.RenderImGUI();
	}
	void Engine::Cmd_UploadMeshes(std::shared_ptr<void> rsc)
	{
		auto* re = (std::vector<imp::CmdRsc::MeshCreationRequest>*)rsc.get();
		// we have place where to add index and vertex components
		// we know vert and idx data

		m_Gfx.CreateAndUploadMeshes(*re);
	}

	void Engine::Cmd_UploadMaterials(std::shared_ptr<void> rsc)
	{
		auto* re = (std::vector<imp::CmdRsc::MaterialCreationRequest>*)rsc.get();
		m_Gfx.CreateAndUploadMaterials(*re);
	}
}