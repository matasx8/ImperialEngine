#version 450

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

struct IndirectDraw
{
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
};

layout(set = 0, binding = 0) readonly buffer Draws
{
	IndirectDraw drawsSrc[];
};

layout(set = 0, binding = 1) writeonly buffer DrawCommands
{
	IndirectDraw drawsDst[];
};

void main()
{
	uint drawIdx = gl_WorkGroupID.x * 32 + gl_LocalInvocationID.x;
    drawsDst[drawIdx] = drawsSrc[drawIdx];
}