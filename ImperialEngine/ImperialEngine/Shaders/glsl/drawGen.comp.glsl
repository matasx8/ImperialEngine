#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

struct IndirectDrawCommand
{
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
};

struct IndirectDraw
{
    uint meshDataIndex;
};

struct BoundingVolume
{
    vec3 center;
    float diameter;
};

struct MeshLOD
{
	uint indexCount;
	uint firstIndex;
};

struct MeshData
{
	MeshLOD LODData[4];
	BoundingVolume boundingVolume;
	int     vertexOffset;
	int     pad;
};

#include "DescriptorSet0.h"

layout(set = 1, binding = 0) readonly buffer Draws
{
	IndirectDraw drawsSrc[];
};

layout(set = 1, binding = 1) writeonly buffer DrawCommands
{
	IndirectDrawCommand drawsDst[];
};

layout(set = 1, binding = 2) readonly buffer MeshDatas
{
    MeshData md[];
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

float distFromCamera = 0.0;

void copy_draw_command(uint idx, uint newIdx)
{
    drawDataIndices[newIdx] = idx;

    MeshData meshdata = md[drawsSrc[idx].meshDataIndex];

    uint lodIdx = 0;
    if(distFromCamera >= 250)
        lodIdx = 3;
    else if(distFromCamera >= 100)
        lodIdx = 2;
    else if(distFromCamera >= 25)
        lodIdx = 1;

    MeshLOD lod = meshdata.LODData[lodIdx];

    drawsDst[newIdx].indexCount    = lod.indexCount;
    drawsDst[newIdx].instanceCount = 1;
    drawsDst[newIdx].firstIndex    = lod.firstIndex;
    drawsDst[newIdx].vertexOffset  = meshdata.vertexOffset;
    drawsDst[newIdx].firstInstance = 0;
}

bool is_inside_view_frustum(uint idx)
{
    BoundingVolume bv = md[drawsSrc[idx].meshDataIndex].boundingVolume;

    vec4 mCenter = vec4(bv.center, 1.0); // BV center in Model space
    vec4 wCenter = drawData[idx].Transform * mCenter;           // BV center in World space

    float diameter = bv.diameter;

    for (int i = 0; i < 6; i++)
    {
        // Compute signed distance of center of BV from plane.
        // Plane equation: Ax + By + Cy + d = 0
        // Inserting our point into equation gives us the signed distance for wCenter
        float signedDistance = dot(frustum[i], wCenter);

        // Negative result already lets us know that point is in negative half space of plane
        // If it's less than 0 + (-diameter) then BV is outside VF
        if(signedDistance < -diameter)
            return false;
        
        // Temporary solution to saving distance from near plane for LOD picking
        if(i == 4)
            distFromCamera = signedDistance - diameter;
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