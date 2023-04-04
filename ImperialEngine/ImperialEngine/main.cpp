#include "frontend/Engine.h"
#include "Utils/EngineStaticConfig.h"
#include "extern/ARGH/argh.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <windows.h>

void ConfigureEngineWithArgs(char** argv, std::vector<std::string>& scenesToLoad, EngineSettings& settings);

#if BENCHMARK_MODE
bool Benchmark(imp::Engine& engine, EngineSettings& settings, int32_t& warmupFrames, int32_t& benchmarkFrames, uint32_t& currRenderModeIdx);
int MergeTimingsAndOutput(imp::Engine& engine);

static constexpr std::array<EngineRenderMode, kEngineRenderModeCount> kRenderModesToBenchmark = { kEngineRenderModeGPUDriven, kEngineRenderModeGPUDrivenMeshShading, kEngineRenderModeTraditional };
#endif

int main(int argc, char** argv)
{
	// Engine is configured to work from ImperialEngine/ImperialEngine/
	if (!IsDebuggerPresent() && std::strstr((const char*)std::filesystem::current_path().c_str(), "x64"))
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

#if BENCHMARK_MODE
	static constexpr uint32_t kWarmUpFrameCount = 20;
	int32_t warmupFrames = kWarmUpFrameCount;
	int32_t benchmarkFrames = settings.gfxSettings.numberOfFramesToBenchmark;

	uint32_t currRenderModeIdx = 0;
	engine.SwitchRenderingMode(kRenderModesToBenchmark[currRenderModeIdx]);
#endif

	// update - sync - render - update
	while (!engine.ShouldClose())
	{
#if BENCHMARK_MODE
		if (Benchmark(engine, settings, warmupFrames, benchmarkFrames, currRenderModeIdx)) break;
#endif
		engine.StartFrame();
		engine.Update();
		engine.SyncGameThread();	// wait for render thread and copy over render data
		engine.Render();			// now give commands to render
		engine.EndFrame();
		engine.SyncRenderThread();	// signal to not expect any more commands and wait at barrier.
									// now while render thread works the main thread can do its work
	}

	// TODO benchmark: seems like the last frames also could use a warmup. Last frame GPU time is x8 for some reason
	engine.ShutDown();

#if BENCHMARK_MODE
	// returns timestamp integer so script can find the correct test dir
	return MergeTimingsAndOutput(engine);
#else
	return 0;
#endif
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

#if BENCHMARK_MODE
bool Benchmark(imp::Engine& engine, EngineSettings& settings, int32_t& warmupFrames, int32_t& benchmarkFrames, uint32_t& currRenderModeIdx)
{
	if (warmupFrames-- == 0)
	{
		engine.StartBenchmark();
	}
	
	if (warmupFrames < 0 && benchmarkFrames-- == 0)
	{
		// probably not needed? or maybe needed for multiple rendering modes
		engine.StopBenchmark();

		warmupFrames = 20;
		benchmarkFrames = settings.gfxSettings.numberOfFramesToBenchmark;

		currRenderModeIdx++;
		if (currRenderModeIdx >= kEngineRenderModeCount)
			return true;

		while (!engine.IsRenderingModeSupported(static_cast<EngineRenderMode>(kRenderModesToBenchmark[currRenderModeIdx])))
		{
			currRenderModeIdx++;
			if (currRenderModeIdx >= kEngineRenderModeCount)
				return true; // cycled through all rendering modes - benchmark done
		}

		engine.SwitchRenderingMode(kRenderModesToBenchmark[currRenderModeIdx]);
	}
	return false;
}

int MergeTimingsAndOutput(imp::Engine& engine)
{
	const auto& mainTables = engine.GetMainBenchmarkTable();
	const auto& renderTables = engine.GetRenderBenchmarkTable();

	std::time_t currTime = time(0);
	std::tm* time = new std::tm();
	localtime_s(time, &currTime);
	const int dateStamp = (time->tm_mon + 1) * 1e8 + time->tm_mday * 1e6 + time->tm_hour * 1e4 + time->tm_min * 1e2 + time->tm_sec;

	std::stringstream outPath;
	outPath << "Testing/TestData/" << std::setfill('0') << std::setw(10) << dateStamp;
	CreateDirectoryA(outPath.str().c_str(), nullptr);

	for (uint32_t i = 0; i < mainTables.size(); i++)
	{
		const auto& mainTable = mainTables[i];
		const auto& renderTable = renderTables[i];

		if (mainTable.table_rows.size() == 0)
			continue;

		std::ofstream file;
		std::stringstream fileName;
		const auto renderModeName = EngineGraphicsSettings::RenderingModeToString(static_cast<EngineRenderMode>(i));
		fileName << outPath.str() << "/TestData-" << renderModeName << ".csv";
		file.open(fileName.str(), std::ios::out);

		if (!file.is_open())
		{
			std::cerr << "[Benchmark]: Failed to open '" << fileName.str() << "' and write test data\n";
			return 0;
		}

		static constexpr char c = ';';
		file << "sep=;" << std::endl;
		file << "Culling" << c << "Drawing" << c << "CPU Main Thread" << c << "CPU Render Thread" << c << "GPU Frame" << c << std::endl;

		for (uint32_t row = 0; row < mainTable.table_rows.size(); row++)
		{
			const auto& mainRow = mainTable.table_rows[row];
			const auto& renderRow = renderTable.table_rows[row];

			double cull = mainRow.cull >= 0.0 ? mainRow.cull : renderRow.cull;
			double draw = mainRow.draw >= 0.0 ? mainRow.draw : renderRow.draw;
			double frameMainCPU = mainRow.frameMainCPU >= 0.0 ? mainRow.frameMainCPU : renderRow.frameMainCPU;
			double frameRenderCPU = mainRow.frameRenderCPU >= 0.0 ? mainRow.frameRenderCPU : renderRow.frameRenderCPU;
			double frameGPU = mainRow.frameGPU >= 0.0 ? mainRow.frameGPU : renderRow.frameGPU;

			file << cull << c << draw << c << frameMainCPU << c << frameRenderCPU << c << frameGPU << c << std::endl;
		}

		file.close();
	}

	return dateStamp;
}
#endif