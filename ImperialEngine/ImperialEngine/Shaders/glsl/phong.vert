#version 450
#extension GL_GOOGLE_include_directive: require
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

layout(location = 0) out vec3 FragPosition OUT_DECLARATION;
layout(location = 1) out vec3 FragNormal   OUT_DECLARATION;
layout(location = 2) out vec2 FragUV	   OUT_DECLARATION;
layout(location = 3) out uint DrawID	   OUT_DECLARATION;

// material data

layout(push_constant) uniform PushModel{
	uint idx;
} pushModel;

void main()
{
	#include "mesh_prefix"

	vec3 pos = vec3(vertices[VERTEX_INDEX].vx, vertices[VERTEX_INDEX].vy, vertices[VERTEX_INDEX].vz);
    vec3 norm = vec3(vertices[VERTEX_INDEX].nx, vertices[VERTEX_INDEX].ny, vertices[VERTEX_INDEX].nz);
    vec2 tex = vec2(vertices[VERTEX_INDEX].tu, vertices[VERTEX_INDEX].tv);

	mat4 model = drawData[nonuniformEXT(DRAW_ID)].Transform;

	FragPosition OUT = (model * vec4(pos, 1.0)).xyz;
	FragNormal   OUT = norm;
	FragUV       OUT = tex;
	DrawID		 OUT = DRAW_ID;

	OUT_POSITION 	= globals.PV * model * vec4(pos, 1.0);

	#include "mesh_postfix"
}