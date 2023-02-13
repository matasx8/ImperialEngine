#version 450 //glsl 4.5
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

layout(location = 0) out float NdotL;
layout(location = 1) out vec3  ReflectVec;
layout(location = 2) out vec3  ViewVec;

#include "DescriptorSet0.h"

layout(push_constant) uniform PushModel{
	uint idx;
} pushModel;
	
void main()
{
	vec3 lol = vec3(materialData[drawData[pushModel.idx].materialIdx].color);
	mat4 model = drawData[pushModel.idx].Transform;
    vec3 ecPos      = vec3(model * vec4(pos, 1.0));
    vec3 tnorm      = norm;
    vec3 lightVec   = normalize(lol  - ecPos);
    ReflectVec      = normalize(reflect(-lightVec, tnorm));
    ViewVec         = normalize(-ecPos);
    NdotL           = (dot(lightVec, tnorm) + 1.0) * 0.5;
	gl_Position 	= uboViewProjection.PV * model * vec4(pos, 1.0);
}