#include "frontend/Engine.h"

namespace imp
{
	void Engine::Cmd_InitGraphics(std::shared_ptr<void> rsc)
	{
		m_Gfx.Initialize(m_EngineSettings.gfxSettings);
	}
}