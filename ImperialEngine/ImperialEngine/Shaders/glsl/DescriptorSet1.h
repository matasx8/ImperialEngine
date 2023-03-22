struct IndirectDrawCommand
{
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
};

struct ms_IndirectDrawCommand
{
    uint    taskCount;
    uint    firstTask;
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

struct ms_MeshLOD
{
    uint meshletBufferOffset;
    uint taskCount;
};

struct MeshData
{
    MeshLOD LODData[4];
    BoundingVolume boundingVolume;
    int     vertexOffset;
    int     pad;
};

struct ms_MeshData
{
    ms_MeshLOD LODData[4];
    BoundingVolume boundingVolume;

    // VkDrawMeshTasksIndirectCommandNV
    uint firstTask;
    uint pad[2];
};

struct NormalCone
{
    vec3 apex;
    int8_t cone[4];
};

struct Meshlet
{
    uint normalConeOffset;
    uint triangleOffset;
    uint vertexOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

layout(set = 1, binding = 0) readonly buffer Draws
{
    IndirectDraw drawsSrc[];
};

layout(set = 1, binding = 1) writeonly buffer DrawCommands
{
#ifndef MESH_PIPELINE
    IndirectDrawCommand drawsDst[];
#else
    //ms_IndirectDrawCommand is smaller in size so using the same buffer for this is ok
    ms_IndirectDrawCommand ms_DrawDst[];
#endif
};

layout(set = 1, binding = 2) readonly buffer MeshDatas
{
    MeshData md[];
};

layout(set = 1, binding = 3) buffer DrawCommandCount
{
    uint drawCommandCount;
};

layout(set = 1, binding = 4) buffer Meshlets
{
    Meshlet meshlets[];
};

layout(set = 1, binding = 5) readonly buffer ms_MeshDatas
{
    ms_MeshData ms_md[];
};

layout(set = 1, binding = 6) readonly buffer ms_MeshletVertexData
{
    uint vertexData[];
};

layout(set = 1, binding = 7) readonly buffer ms_MeshletTriangleData
{
    uint triangleData[];
};

layout(set = 1, binding = 8) readonly buffer ms_MeshletConeData
{
    NormalCone normalCone[];
};