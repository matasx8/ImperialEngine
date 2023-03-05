#version 450 //glsl 4.5
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_shader_draw_parameters: require
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_shader_16bit_storage: require

layout(location = 0) out float NdotL;
layout(location = 1) out vec3  ReflectVec;
layout(location = 2) out vec3  ViewVec;

#include "DescriptorSet0.h"
	
layout(push_constant) uniform PushModel{
	uint idx;
} pushModel;
	
void main()
{
    vec3 pos = vec3(vertices[gl_VertexIndex].vx, vertices[gl_VertexIndex].vy, vertices[gl_VertexIndex].vz);
    vec3 norm = vec3(vertices[gl_VertexIndex].nx, vertices[gl_VertexIndex].ny, vertices[gl_VertexIndex].nz);
    vec2 tex = vec2(vertices[gl_VertexIndex].tu, vertices[gl_VertexIndex].tv);

	vec3 lol = vec3(materialData[drawData[drawDataIndices[gl_DrawIDARB]].materialIdx].color);
	mat4 model = drawData[drawDataIndices[gl_DrawIDARB]].Transform;
    vec3 ecPos      = vec3(model * vec4(pos, 1.0));
    vec3 tnorm      = norm;
    vec3 lightVec   = normalize(lol  - ecPos);
    ReflectVec      = normalize(reflect(-lightVec, tnorm));
    ViewVec         = normalize(-ecPos);
    NdotL           = (dot(lightVec, tnorm) + 1.0) * 0.5;
	gl_Position 	= uboViewProjection.PV * model * vec4(pos, 1.0);
}