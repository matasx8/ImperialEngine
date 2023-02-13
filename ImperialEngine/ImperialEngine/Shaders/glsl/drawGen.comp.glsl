#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

// actual indirect draw command that command processor will use
// VkDrawIndexedIndirectCommand
// write to
struct IndirectDrawCommand
{
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
};

// read from
struct IndirectDraw
{
    IndirectDrawCommand idc;
    uint    BVIndex;        // index to binding volume descriptor
};

struct BoundingVolume
{
    vec3 center;
    float radius;
};

layout(set = 0, binding = 0) uniform UboViewProjection{
	mat4 PV;
} uboViewProjection;

layout(set=0, binding = 1) buffer MaterialData
{
	vec4 color;
} materialData[128];

// use gl_DrawIDARB to find this draw index into DrawData.
// This is needed because after culling there isn't a 1:1 ratio to IndirectDrawCommands and DrawData
layout(set=0, binding = 129) buffer DrawDataIndices
{
	uint drawDataIndices[];
};

layout (set = 0, binding = 130) buffer DrawData
{
	mat4 Transform;
	uint materialIdx;
} drawData[];

layout(set = 1, binding = 0) readonly buffer Draws
{
	IndirectDraw drawsSrc[];
};

layout(set = 1, binding = 1) writeonly buffer DrawCommands
{
	IndirectDrawCommand drawsDst[];
};

layout(set = 1, binding = 2) readonly buffer BoundingVolumes
{
    BoundingVolume bv[];
};

layout(set = 1, binding = 3) buffer DrawCommandCount
{
    uint drawCommandCount;
};

layout(push_constant) uniform ViewFrustum
{
    vec4 frustum[6];
    uint numDraws;
};

void copy_draw_command(uint idx, uint newIdx)
{
    drawDataIndices[newIdx] = idx;
    drawsDst[newIdx].indexCount    = drawsSrc[idx].idc.indexCount;
    drawsDst[newIdx].instanceCount = drawsSrc[idx].idc.instanceCount;
    drawsDst[newIdx].firstIndex    = drawsSrc[idx].idc.firstIndex;
    drawsDst[newIdx].vertexOffset  = drawsSrc[idx].idc.vertexOffset;
    drawsDst[newIdx].firstInstance = drawsSrc[idx].idc.firstInstance;
}

bool is_inside_view_frustum(uint idx)
{
    vec4 mCenter = vec4(bv[drawsSrc[idx].BVIndex].center, 1.0); // BV center in Model space
    vec4 wCenter = drawData[idx].Transform * mCenter;           // BV center in World space

    float radius = bv[drawsSrc[idx].BVIndex].radius;

    for (int i = 0; i < 6; i++)
    {
        if(dot(frustum[i], wCenter) < -radius)
            return false;
    }
        return true;
}

void main()
{
	uint drawIdx = gl_WorkGroupID.x * 32 + gl_LocalInvocationID.x;

    if(drawIdx >= numDraws)
        return;

    bool isVisible = is_inside_view_frustum(drawIdx);

    if(isVisible)
    {
        uint newDrawIndex = atomicAdd(drawCommandCount, 1);
        copy_draw_command(drawIdx, newDrawIndex);
    }
}