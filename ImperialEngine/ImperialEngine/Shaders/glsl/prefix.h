#ifndef DEBUG_MESH
#define DEBUG_MESH 1
#endif

#ifndef LOD_ENABLED
#define LOD_ENABLED 0
#endif

#if LOD_ENABLED
#define MESH_LOD_COUNT 4
#else
#define MESH_LOD_COUNT 1
#endif

// if disabled also disables task shader
#ifndef CONE_CULLING_ENABLED
#define CONE_CULLING_ENABLED 0
#endif 