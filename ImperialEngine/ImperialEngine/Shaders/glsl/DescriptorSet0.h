layout(set = 0, binding = 0) uniform UboViewProjection {
	mat4 PV;
} uboViewProjection;

layout(set = 0, binding = 1) buffer MaterialData
{
	vec4 color;
} materialData[128];

// use gl_DrawIDARB to find this draw index into DrawData.
// This is needed because after culling there isn't a 1:1 ratio to IndirectDrawCommands and DrawData
layout(set = 0, binding = 129) buffer DrawDataIndices
{
	uint drawDataIndices[];
};

layout(set = 0, binding = 130) buffer DrawData
{
	mat4 Transform;
	uint materialIdx;
} drawData[];