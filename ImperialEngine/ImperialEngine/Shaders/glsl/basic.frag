#version 450
 
layout(location = 0) out vec4 outColour;

layout(location = 0) in float NdotL;
layout(location = 1) in vec3  ReflectVec;
layout(location = 2) in vec3  ViewVec;

void main()
{
	vec3  SurfaceColor = vec3(0.75, 0.75, 0.75);
	vec3  WarmColor    = vec3(0.6, 0.6, 0.0);
	vec3  CoolColor    = vec3(0.0, 0.0, 0.6);
	float DiffuseWarm =  0.45;
	float DiffuseCool = 0.45;
	
    vec3 kcool    = min(CoolColor + DiffuseCool * SurfaceColor, 1.0);
    vec3 kwarm    = min(WarmColor + DiffuseWarm * SurfaceColor, 1.0); 
    vec3 kfinal   = mix(kcool, kwarm, NdotL);

    vec3 nreflect = normalize(ReflectVec);
    vec3 nview    = normalize(ViewVec);

    float spec    = max(dot(nreflect, nview), 0.0);
    spec          = pow(spec, 32.0);

    outColour = vec4(min(kfinal + spec, 1.0), 1.0);
}