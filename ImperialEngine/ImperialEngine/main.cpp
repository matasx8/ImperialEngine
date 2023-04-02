#include "frontend/Engine.h"
#include "Utils/EngineStaticConfig.h"
#include "extern/ARGH/argh.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h>

void ConfigureEngineWithArgs(char** argv, std::vector<std::string>& scenesToLoad, EngineSettings& settings);

int main(int argc, char** argv)
{
	// Engine is configured to work from ImperialEngine/ImperialEngine/
	if (!IsDebuggerPresent())
		std::filesystem::current_path("../../../ImperialEngine");

	EngineSettings settings;
#if _DEBUG && !_DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDebug };
#elif _DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDevelopment };
#elif NDEBUG
	settings = { EngineSettingsTemplate::kEngineSettingsRelease };
#endif
	imp::Engine engine;

	std::vector<std::string> scenesToLoad;
	ConfigureEngineWithArgs(argv, scenesToLoad, settings);

	if (!engine.Initialize(settings))
		return 1;

	engine.LoadScenes(scenesToLoad);
	engine.LoadAssets();

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

void ConfigureEngineWithArgs(char** argv, std::vector<std::string>& scenesToLoad, EngineSettings& settings)
{
	argh::parser cmdl(argv);

	if (cmdl["--wait-for-debugger"])
	{
		// for some reason debugbreak just terminates..
		// so just wait 10s
		Sleep(1000 * 10);
	}

#if BENCHMARK_MODE
	cmdl("--run-for") >> settings.gfxSettings.numberOfFramesToBenchmark;
#endif

	auto lfIdx = std::find(cmdl.args().begin(), cmdl.args().end(), "--load-files");
	if (lfIdx != cmdl.args().end())
	{
		int numFilesToLoad;
		cmdl("--file-count") >> numFilesToLoad;

		lfIdx++;
		for (int i = 0; i < numFilesToLoad; i++)
		{
			const auto path = *lfIdx;
			scenesToLoad.push_back(path);
			lfIdx++;
		}

	}
}
