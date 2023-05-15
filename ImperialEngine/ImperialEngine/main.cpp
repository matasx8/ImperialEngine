#include "frontend/Engine.h"
#include "Utils/EngineStaticConfig.h"
#include "extern/ARGH/argh.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <windows.h>
#include <GLM/gtx/quaternion.hpp>

struct CLI
{
	std::vector<std::string> scenesToLoad;
	std::string distribution;
	std::string entityCount;
	std::string cameraMovement;
	std::string growthStep;
};

bool ConfigureEngineWithArgs(char** argv, CLI& cli, EngineSettings& settings);
void CustomUpdates(imp::Engine& engine, const CLI& cli);

#if BENCHMARK_MODE
bool Benchmark(imp::Engine& engine, const CLI& cli, EngineSettings& settings, int32_t& warmupFrames, int32_t& benchmarkFrames, uint32_t& currRenderModeIdx);
int MergeTimingsAndOutput(imp::Engine& engine);

static constexpr std::array<EngineRenderMode, kEngineRenderModeCount> kRenderModesToBenchmark = { kEngineRenderModeGPUDriven, kEngineRenderModeGPUDrivenMeshShading, kEngineRenderModeTraditional };
#endif

int main(int argc, char** argv)
{
	// Engine is configured to work from ImperialEngine/ImperialEngine/
	if (std::filesystem::current_path().string().find("x64") != std::string::npos)
		std::filesystem::current_path("../../../ImperialEngine");

	printf("Current wdir: %s\n", std::filesystem::current_path().string().c_str());

	EngineSettings settings;
#if _DEBUG && !_DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDebug };
#elif _DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDevelopment };
#elif NDEBUG
	settings = { EngineSettingsTemplate::kEngineSettingsRelease };
#endif
	imp::Engine engine;

	CLI cli;
	if (!ConfigureEngineWithArgs(argv, cli, settings))
		return 1;

	if (!engine.Initialize(settings))
		return 1;

	engine.LoadScenes(cli.scenesToLoad);
	engine.LoadAssets();

	if (cli.distribution.size() && cli.entityCount.size())
		engine.DistributeEntities(cli.distribution, cli.entityCount);

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
		if (Benchmark(engine, cli, settings, warmupFrames, benchmarkFrames, currRenderModeIdx)) break;
#endif
		engine.StartFrame();
#if BENCHMARK_MODE
		CustomUpdates(engine, cli);
#endif
		engine.Update();
		engine.SyncGameThread();	// wait for render thread and copy over render data
		engine.Render();			// now give commands to render
		engine.EndFrame();
		engine.SyncRenderThread();	// signal to not expect any more commands and wait at barrier.
									// now while render thread works the main thread can do its work
	}

	engine.ShutDown();

#if BENCHMARK_MODE
	// returns timestamp integer so script can find the correct test dir
	return MergeTimingsAndOutput(engine);
#else
	return 0;
#endif
}

static void PrintCorrectCLI()
{
	printf("ImperialEngine.exe [--wait-for-debugger] [--file-count=<count>] [--load-files <file names>] [--entity-count=<count>] [--distribute=<distribution>]\n");
}

bool ConfigureEngineWithArgs(char** argv, CLI& cli, EngineSettings& settings)
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

	if (cmdl("--distribute"))
	{
		cli.distribution = cmdl("--distribute").str();
		if (cli.distribution != "random")
		{
			printf("[CLI]: Error! only 'random' distribution is supported\n");
			PrintCorrectCLI();
			return false;
		}
	}

	if (cmdl("--entity-count"))
	{
		cli.entityCount = cmdl("--entity-count").str();
		
		if (cli.entityCount != "max")
		{
			const auto numEntities = int32_t(std::stoul(cli.entityCount));
			if (numEntities < 0)
			{
				printf("[CLI]: Error! entity-count must be a positive integer or 'max'\n");
				PrintCorrectCLI();
				return false;
			}
		}
	}

	if (cmdl("--camera-movement"))
		cli.cameraMovement = cmdl("--camera-movement").str();

	if (cmdl("--growth-step"))
		cli.growthStep = cmdl("--growth-step").str();

	auto lfIdx = std::find(cmdl.args().begin(), cmdl.args().end(), "--load-files");
	if (lfIdx != cmdl.args().end())
	{
		int numFilesToLoad;
		cmdl("--file-count") >> numFilesToLoad;

		if (numFilesToLoad < 1)
		{
			printf("[CLI]: Error! file-count must be a positive integer\n");
			PrintCorrectCLI();
			return false;
		}

		lfIdx++;
		for (int i = 0; i < numFilesToLoad; i++)
		{
			const auto path = *lfIdx;
			cli.scenesToLoad.push_back(path);
			lfIdx++;
		}
	}
	return true;
}

void CustomUpdates(imp::Engine& engine, const CLI& cli)
{
	auto& reg = engine.GetEntityRegistry();
	const auto cameras = reg.view<Comp::Transform, Comp::Camera>();
	if (cli.cameraMovement == "rotate")
	{
		constexpr float rotationStep = glm::radians(360.0f / 1000.0f);

		for (auto ent : cameras)
		{
			auto& transform = cameras.get<Comp::Transform>(ent);

			const auto axis = glm::vec3(0.0f, 1.0f, 0.0f);
			transform.transform = glm::rotate(transform.transform, rotationStep, axis);
		}
	}
	else if (cli.cameraMovement == "away")
	{
		constexpr glm::vec3 awayStep = glm::vec3(0.0f, 0.0f, 0.5f);

		for (auto ent : cameras)
		{
			auto& transform = cameras.get<Comp::Transform>(ent);

			const auto axis = glm::vec3(0.0f, 1.0f, 0.0f);
			transform.transform = glm::translate(transform.transform, awayStep);
		}
	}

	if (cli.growthStep.size())
	{
		engine.DistributeEntities(cli.distribution, cli.growthStep);
	}
}

#if BENCHMARK_MODE
bool Benchmark(imp::Engine& engine, const CLI& cli, EngineSettings& settings, int32_t& warmupFrames, int32_t& benchmarkFrames, uint32_t& currRenderModeIdx)
{
	if (warmupFrames-- == 0)
	{
		engine.StartBenchmark();
	}
	
	if (warmupFrames < 0 && benchmarkFrames-- == 0)
	{
		engine.StopBenchmark();

		warmupFrames = 20;
		benchmarkFrames = settings.gfxSettings.numberOfFramesToBenchmark;

		auto& reg = engine.GetEntityRegistry();
		auto renderableChildren = reg.view<Comp::ChildComponent, Comp::Mesh, Comp::Material>();
		auto transforms = reg.view<Comp::Transform>();

		if (cli.growthStep.size())
		{	// if we're doing growth step then we have to remove entities after each benchmark
			std::vector<entt::entity> entitiesToRemove;
			for (auto ent : renderableChildren)
			{
				entitiesToRemove.push_back(ent);
				const auto& parent = renderableChildren.get<Comp::ChildComponent>(ent).parent;
				entitiesToRemove.push_back(parent);
			}
			reg.destroy(entitiesToRemove.begin(), entitiesToRemove.end());
		}


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
		file << "Culling" << c << "Frame Time" << c << "CPU Main Thread" << c << "CPU Render Thread" << c << "GPU Frame" << c << "Triangles" << std::endl;

		for (uint32_t row = 0; row < mainTable.table_rows.size(); row++)
		{
			const auto& mainRow = mainTable.table_rows[row];
			const auto& renderRow = renderTable.table_rows[row];

			double cull = mainRow.cull >= 0.0 ? mainRow.cull : renderRow.cull;
			double draw = mainRow.frame >= 0.0 ? mainRow.frame : renderRow.frame;
			double frameMainCPU = mainRow.frameMainCPU >= 0.0 ? mainRow.frameMainCPU : renderRow.frameMainCPU;
			double frameRenderCPU = mainRow.frameRenderCPU >= 0.0 ? mainRow.frameRenderCPU : renderRow.frameRenderCPU;
			double frameGPU = mainRow.frameGPU >= 0.0 ? mainRow.frameGPU : renderRow.frameGPU;
			int64_t triangles = renderRow.triangles;

			file << cull << c << draw << c << frameMainCPU << c << frameRenderCPU << c << frameGPU << c << triangles << std::endl;
		}

		file.close();
	}

	return dateStamp;
}
#endif