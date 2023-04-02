#pragma once
#include <string>
#include <vector>

enum class EngineSettingsTemplate
{
	kEngineSettingsDebug, // abandoned
	kEngineSettingsDevelopment, // debug and multithread
	kEngineSettingsRelease
};

enum EngineThreadingMode
{
	kEngineSingleThreaded,
	kEngineMultiThreaded
};

enum EngineSwapchainImageCount
{
	kEngineSwapchainDoubleBuffering = 2,
	kEngineSwapchainTripleBuffering
};

enum EnginePresentMode
{
	kEnginePresentFifo,
	kEnginePresentMailbox
};

enum EngineRenderMode : uint32_t
{
	// "Traditional" Drawcall submission on the CPU
	kEngineRenderModeTraditional,

	// GPU-Driven approach using Indirect Draws in a GPU buffer
	kEngineRenderModeGPUDriven,

	// GPU-Driven approach using Mesh Shading
	kEngineRenderModeGPUDrivenMeshShading,

	// Number of different "rendering" modes
	kEngineRenderModeCount
};

inline constexpr uint32_t kDefaultEngineRenderMode = kEngineRenderModeGPUDriven;

struct EngineGraphicsSettings
{
	std::vector<const char*> requiredExtensions;
	std::vector<const char*> requiredDeviceExtensions;
	std::vector<EnginePresentMode> preferredPresentModes;	// sorted list of preferred present modes, first available is chosen
	EngineSwapchainImageCount swapchainImageCount;
	EngineRenderMode renderMode;
	bool validationLayersEnabled;

	uint32_t numberOfFramesToBenchmark;
};

// engine settings used for initialization
// can configure everything yourself or use a predefined template configured by EngineSettingsTemplate
struct EngineSettings
{
	EngineSettings();
	EngineSettings(EngineSettingsTemplate settingsTemplate);
	
	EngineThreadingMode threadingMode;
	EngineGraphicsSettings gfxSettings;
};

