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

struct Meshlet
{
    uint vertices[64];
    uint8_t indices[126 * 3]; // up to 126 triangles
    uint8_t triangleCount;
    uint8_t vertexCount;
};

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

layout(set = 1, binding = 4) buffer Meshlets
{
    Meshlet meshlets[];
};