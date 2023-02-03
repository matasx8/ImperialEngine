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

// "global" data
layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 PV;
} uboViewProjection;

layout(set=0, binding = 1) buffer MaterialData
{
	vec4 color;
} materialData[128];

// information for each draw (mostly indexes to other resource descriptors)
layout (set = 0, binding = 129) buffer DrawData
{
	mat4 Transform;
	uint materialIdx;	// index to MaterialData
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

layout(push_constant) uniform ViewFrustum
{
    vec4 frustum[6];
	// Data about the VF.
    // maybe don't deconstruct in CS since it's harder to debug
};

void copy_draw_command(uint idx, int isInsideVF)
{
    drawsDst[idx].indexCount    = drawsSrc[idx].idc.indexCount;
    drawsDst[idx].instanceCount = isInsideVF;
    drawsDst[idx].firstIndex    = drawsSrc[idx].idc.firstIndex;
    drawsDst[idx].vertexOffset  = drawsSrc[idx].idc.vertexOffset;
    drawsDst[idx].firstInstance = drawsSrc[idx].idc.firstInstance;
}

int is_inside_view_frustum(uint idx)
{
    vec3 center = (drawData[idx].Transform * vec4(bv[drawsSrc[idx].BVIndex].center, 1.0)).xyz;
    //vec3 center = bv[drawsSrc[idx].BVIndex].center;
    float radius = bv[drawsSrc[idx].BVIndex].radius;
    for (int i = 0; i < 4; i++)
    {
        if(dot(frustum[i], vec4(center, 1)) < 0)
            return 0;
    }
        return 1;
}

void main()
{
	uint drawIdx = gl_WorkGroupID.x * 32 + gl_LocalInvocationID.x;

    copy_draw_command(drawIdx, is_inside_view_frustum(drawIdx));
}