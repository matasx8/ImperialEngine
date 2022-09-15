#version 450 //glsl 4.5
 
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

layout(location = 0) out float NdotL;
layout(location = 1) out vec3  ReflectVec;
layout(location = 2) out vec3  ViewVec;

layout(set = 0, binding = 0) uniform UboViewProjection{
	mat4 PV;
} uboViewProjection;

layout(push_constant) uniform PushModel{
	mat4 model;
} pushModel;
	
void main()
{
    vec3 ecPos      = vec3(pushModel.model * vec4(pos, 1.0));
    vec3 tnorm      = norm;
    vec3 lightVec   = normalize(vec3(0.0, 10.0, 4.0)  - ecPos);
    ReflectVec      = normalize(reflect(-lightVec, tnorm));
    ViewVec         = normalize(-ecPos);
    NdotL           = (dot(lightVec, tnorm) + 1.0) * 0.5;
   // gl_Position     = ftransform();
	gl_Position 	= uboViewProjection.PV * pushModel.model * vec4(pos, 1.0);
	gl_Position.y 	= -gl_Position.y;
}