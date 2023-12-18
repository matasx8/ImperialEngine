layout(set = 0, binding = 0) uniform Globals 
{
	mat4 PV;
	mat4 View;
	mat4 cameraModel;
	vec4 frustum[6];
	vec4 lightPositon;
} globals;

struct Vertex
{
	float vx, vy, vz;
	float16_t nx, ny, nz, nw;
	float16_t tu, tv;
};

layout(set = 0, binding = 1) readonly buffer Vertices
{
	Vertex vertices[];
};

layout(set = 0, binding = 2) buffer MaterialData
{
	uint albedoIdx;
} materialData[MAX_MATERIALS];

layout(set = 0, binding = 130) uniform sampler2D samplerData[];

// use gl_DrawIDARB to find this draw index into DrawData.
// This is needed because after culling there isn't a 1:1 ratio to IndirectDrawCommands and DrawData
layout(set = 0, binding = 258) buffer DrawDataIndices
{
	uint drawDataIndices[];
};

layout(set = 0, binding = 259) buffer DrawData
{
	mat4 Transform;
	uint materialIdx;
	uint vertexBufferOffset;
} drawData[];