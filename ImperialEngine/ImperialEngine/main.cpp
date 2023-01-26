#include<iostream>
#include "frontend/Engine.h"
#include <chrono>
#include <thread>

int main()
{
	EngineSettings settings;
#if _DEBUG && !_DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDebug };
#elif _DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDevelopment };
#elif NDEBUG
	settings = { EngineSettingsTemplate::kEngineSettingsRelease };
#endif
	imp::Engine engine;
	
	if (!engine.Initialize(settings))
		return 1;

	// load stuff
	engine.LoadScene();

	engine.SyncRenderThread();

	// update - sync - render - update
	while (!engine.ShouldClose())
	{
		engine.StartFrame();
		engine.Update();
		engine.SyncGameThread();	// wait for render thread and copy over render data
		engine.Render();			// now give commands to render
		engine.EndFrame();
		engine.SyncRenderThread();	// signal to not expect any more commands and wait at barrier.
									// now while render thread works the main thread can do its work
	}

	engine.ShutDown();
	return 0;
}