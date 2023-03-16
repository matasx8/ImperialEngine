#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define MESH_PIPELINE
#include "DescriptorSet0.h"
#include "DescriptorSet1.h"

// TODO mesh: remove .glsl postfix of compute files

layout(push_constant) uniform ViewFrustum
{
    vec4 frustum[6];
    uint numDraws;
};

float distFromCamera = 0.0;

void copy_draw_command(uint idx, uint newIdx)
{
    drawDataIndices[newIdx] = idx;
    
    ms_MeshData meshdata = ms_md[drawsSrc[idx].meshDataIndex];
    
    ms_DrawDst[newIdx].taskCount = (meshdata.taskCount + 31) / 32;
    ms_DrawDst[newIdx].firstTask = 0;
}

bool is_inside_view_frustum(uint idx)
{
    BoundingVolume bv = ms_md[drawsSrc[idx].meshDataIndex].boundingVolume;

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
	uint drawIdx = gl_WorkGroupID.x * 64 + gl_LocalInvocationID.x;

    if(drawIdx >= numDraws)
        return;

    bool isVisible = is_inside_view_frustum(drawIdx);

    if(isVisible)
    {
        uint newDrawIndex = atomicAdd(drawCommandCount, 1);
        copy_draw_command(drawIdx, newDrawIndex);
    }
}