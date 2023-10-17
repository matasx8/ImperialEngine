#version 450
#extension GL_GOOGLE_include_directive: require
#include "prefix.h"
#include "utils.h"

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage: require

#include "DescriptorSet0.h"
#include "DescriptorSet3.h"
 
layout(location = 0) out vec4 outColour;

layout(location = 0) in vec3 FragPosition;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragUV;
layout(location = 3) flat in uint DrawID;

void main()
{
	uint materialID = drawData[nonuniformEXT(DrawID)].materialIdx;
	uint samplerID = materialData[materialID].albedoIdx;
	vec4 colorRGBA = texture(samplerData[nonuniformEXT(samplerID)], FragUV);

	// Calculate ambient lighting
	vec3 ambient = 0.05 * colorRGBA.rgb;

	// Calculate diffuse lighting
	vec3 lightDir = normalize(globals.lightPositon.xyz - FragPosition);
	vec3 normal = normalize(FragNormal); 	// Why do we normalize here when we could do it in VS? Do normalized stuff not interpolate well?
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * colorRGBA.rgb;

	// Find the view direction and reflection direction
	vec3 viewPos = GET_POSITION(globals.cameraModel).xyz;
	vec3 viewDir = normalize(viewPos - FragPosition);
	vec3 reflectDir = reflect(-lightDir, normal);

	// Calculate the specular highlight
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
	vec3 specular = vec3(0.3) * spec;

	// Output final color
	outColour = vec4(ambient + diffuse + specular, 1.0);
}