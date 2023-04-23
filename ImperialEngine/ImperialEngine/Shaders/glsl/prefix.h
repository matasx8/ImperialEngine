#ifndef DEBUG_MESH
#define DEBUG_MESH 1
#endif

#ifndef LOD_ENABLED
#define LOD_ENABLED 1
#endif

#if LOD_ENABLED
#define MESH_LOD_COUNT 4
#else
#define MESH_LOD_COUNT 1
#endif

// if disabled also disables task shader
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