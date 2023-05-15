#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define MESH_PIPELINE
#include "prefix.h"
#include "DescriptorSet0.h"
#include "DescriptorSet1.h"

// TODO mesh: remove .glsl postfix of compute files

layout(push_constant) uniform ViewFrustum
{
    uint numDraws;
};

float distFromCamera = 0.0;

void copy_draw_command(uint idx, uint newIdx)
{
    drawDataIndices[newIdx] = idx;
    
    ms_MeshData meshdata = ms_md[drawsSrc[idx].meshDataIndex];

    uint lodIdx = 0;
#if LOD_ENABLED
    if(distFromCamera >= 250)
        lodIdx = 3;
    else if(distFromCamera >= 100)
        lodIdx = 2;
    else if(distFromCamera >= 25)
        lodIdx = 1;
#endif
    
    ms_MeshLOD lod = meshdata.LODData[lodIdx];
    
#if CONE_CULLING_ENABLED
    ms_DrawDst[newIdx].taskCount = (lod.taskCount + MESH_WGROUP - 1) / MESH_WGROUP;
#else
    ms_DrawDst[newIdx].taskCount = lod.taskCount;
#endif

    ms_DrawDst[newIdx].firstTask = 0;

    ms_DrawDst[newIdx].meshletBufferOffset = lod.meshletBufferOffset;
    ms_DrawDst[newIdx].meshTaskCount = lod.taskCount;
}

bool is_inside_view_frustum(uint idx)
{
    BoundingVolume bv = ms_md[drawsSrc[idx].meshDataIndex].boundingVolume;

    vec4 mCenter = vec4(bv.center, 1.0); // BV center in Model space
    vec4 wCenter = drawData[idx].Transform * mCenter;           // BV center in World space

    float scale = length(drawData[idx].Transform[0].xyz);

    float radius = bv.radius * scale;

    for (int i = 0; i < 6; i++)
    {
        // Compute signed distance of center of BV from plane.
        // Plane equation: Ax + By + Cy + d = 0
        // Inserting our point into equation gives us the signed distance for wCenter
        float signedDistance = dot(globals.frustum[i], wCenter);

        // Negative result already lets us know that point is in negative half space of plane
        // If it's less than 0 + (-radius) then BV is outside VF
        if(signedDistance < -radius)
            return false;
        
#if LOD_ENABLED
        // Temporary solution to saving distance from near plane for LOD picking
        if(i == 4)
            distFromCamera = signedDistance - radius;
#endif
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