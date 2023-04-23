#pragma once

// CPU culling
#ifndef CPU_CULL_ST
#define CPU_CULL_ST 1
#endif

// Benchmarking mode. Engine will run test scene with each available rendering mode, collect data and output it, then shut down.
// UI will be disabled.
#ifndef BENCHMARK_MODE
#define BENCHMARK_MODE 0
#endif

#ifndef USE_AFTERMATH
#define USE_AFTERMATH 0
#endif

#ifndef LOG_TIMINGS
#define LOG_TIMINGS 0
#endif

#ifndef LOD_ENABLED
#define LOD_ENABLED 1
#endif

#ifndef CULLING_ENABLED
#define CULLING_ENABLED 1
#endif

// if disabled also disabled task shader
#ifndef CONE_CULLING_ENABLED
#define CONE_CULLING_ENABLED 1
#endif 

#ifndef MESHLET_MAX_PRIMS
#define MESHLET_MAX_PRIMS 84
#endif 

#ifndef MESHLET_MAX_VERTS
#define MESHLET_MAX_VERTS 64
#endif

#ifndef MESH_WGROUP
#define MESH_WGROUP 32
#endif