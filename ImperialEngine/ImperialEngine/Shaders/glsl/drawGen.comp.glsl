#version 450

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

//layout(set = 1, binding = 2) readonly buffer BoundingVolume
//{
//    // BV data
//} boundingVolumes[]

layout(push_constant) uniform ViewFrustum
{
    uint arbitraryData;
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
    //TransformedBV = drawData[idx].Transform * BV;
    //if (TransformedBV is outside VF)
    //    return 0;
    //else
        return 1;
}

void main()
{
	uint drawIdx = gl_WorkGroupID.x * 32 + gl_LocalInvocationID.x;

    copy_draw_command(drawIdx, is_inside_view_frustum(0));
}