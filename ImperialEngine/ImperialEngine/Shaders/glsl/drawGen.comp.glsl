#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "DescriptorSet0.h"
#include "DescriptorSet1.h"

layout(push_constant) uniform ViewFrustum
{
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

    float scale = length(drawData[idx].Transform[0].xyz);

    // dumbass its supposed to be radius..
    float diameter = bv.diameter * scale;

    for (int i = 0; i < 6; i++)
    {
        // Compute signed distance of center of BV from plane.
        // Plane equation: Ax + By + Cz + d = 0
        // Inserting our point into equation gives us the signed distance for wCenter
        float signedDistance = dot(globals.frustum[i], wCenter);

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