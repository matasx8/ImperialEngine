#include "frontend/Engine.h"

namespace imp
{
	void Engine::Cmd_InitGraphics(std::shared_ptr<void> rsc)
	{
		auto* re = (Window*)rsc.get();
		m_Gfx.Initialize(m_EngineSettings.gfxSettings, re);
	}
}