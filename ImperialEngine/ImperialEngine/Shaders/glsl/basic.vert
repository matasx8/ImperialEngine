#version 450 //glsl 4.5
#include "prefix.h"
#include "pipeline_compatibility.h"

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage: require

#if INDIRECT_PIPELINE || MESH_PIPELINE
#extension GL_ARB_shader_draw_parameters: require
#endif

#if MESH_PIPELINE
#extension GL_NV_mesh_shader: require
#extension GL_EXT_shader_8bit_storage: require
#endif

#include "DescriptorSet0.h"
#if MESH_PIPELINE
#include "DescriptorSet1.h"
#endif

#include "math.h"

#if MESH_PIPELINE
layout(local_size_x = MESH_WGROUP, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = MESHLET_MAX_VERTS, max_primitives = MESHLET_MAX_PRIMS) out;

#if CONE_CULLING_ENABLED
in taskNV block
{
	uint meshletIndices[MESH_WGROUP];
	uint drawIdx;
};
#endif
#endif

layout(location = 0) out float NdotL        OUT_DECLARATION;
layout(location = 1) out vec3  ReflectVec   OUT_DECLARATION;
layout(location = 2) out vec3  ViewVec      OUT_DECLARATION;
#if DEBUG_MESH
layout(location = 3) out vec3 SurfaceColor  OUT_DECLARATION;
#endif


layout(push_constant) uniform PushModel{
	uint idx;
} pushModel;
	
void main()
{
	#include "mesh_prefix"

    vec3 pos = vec3(vertices[VERTEX_INDEX].vx, vertices[VERTEX_INDEX].vy, vertices[VERTEX_INDEX].vz);
    vec3 norm = vec3(vertices[VERTEX_INDEX].nx, vertices[VERTEX_INDEX].ny, vertices[VERTEX_INDEX].nz);
    vec2 tex = vec2(vertices[VERTEX_INDEX].tu, vertices[VERTEX_INDEX].tv);

	vec3 color = vec3(materialData[drawData[DRAW_ID].materialIdx].color);
	mat4 model = drawData[DRAW_ID].Transform;
    vec3 ecPos      = vec3(model * vec4(pos, 1.0));
    vec3 tnorm      = norm;
    vec3 lightVec   = normalize(color  - ecPos);
    ReflectVec  OUT = normalize(reflect(-lightVec, tnorm));
    ViewVec     OUT = normalize(-ecPos);
    NdotL       OUT = (dot(lightVec, tnorm) + 1.0) * 0.5;
#if DEBUG_MESH
    #if MESH_PIPELINE
    uint mhash = hash(mi);
	vec3 mcolor = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;
	SurfaceColor OUT = mcolor;
    #else
    SurfaceColor OUT = vec3(0.3, 0.3, 0.3);
    #endif
#endif
	OUT_POSITION 	= globals.PV * model * vec4(pos, 1.0);

    #include "mesh_postfix"
}