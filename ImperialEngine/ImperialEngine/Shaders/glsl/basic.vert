#version 450 //glsl 4.5
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

layout(location = 0) out float NdotL;
layout(location = 1) out vec3  ReflectVec;
layout(location = 2) out vec3  ViewVec;

layout(set = 0, binding = 0) uniform UboViewProjection{
	mat4 PV;
} uboViewProjection;

layout(set=0, binding = 1) buffer MaterialData
{
	vec4 color;
} materialData[128];


layout(set=0, binding = 129) buffer DrawDataIndices
{
	uint drawDataIndices[];
};

layout (set = 0, binding = 130) buffer DrawData
{
	mat4 Transform;
	uint materialIdx;
	//uint isEnabled;
	//uint pad;
	//uint pad2;
} drawData[];

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