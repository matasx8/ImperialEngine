#include "frontend/Engine.h"

namespace imp
{
	void Engine::Cmd_InitGraphics(std::shared_ptr<void> rsc)
	{
		auto* re = (Window*)rsc.get();
		m_Gfx.Initialize(m_EngineSettings.gfxSettings, re);
		// after initing graphics we can now wait for first update
		m_SyncPoint->arrive_and_wait();
	}

	void Engine::Cmd_RenderCameras(std::shared_ptr<void> rsc)
	{
		// for each camera
		// camera.Render();
		// -- Render() --
		// Camera has a list of render passes
		// render pass has info to start rp and bind framebuffer
		// get all renderer components and draw them 
		// --
		// Surface manager will have to take and give surfaces
		// camera with its stack of render passes will have to pass on surfaces to next render passes
		m_Gfx.PrototypeRenderPass();
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
		auto* re = (std::vector<imp::CmdRsc::MeshCreationRequest>*)rsc.get();
		// we have place where to add index and vertex components
		// we know vert and idx data

		m_Gfx.CreateAndUploadMeshes(*re);
	}
}