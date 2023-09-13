// TRADITIONAL_PIPELINE - no indirect stuff
// INDIRECT_PIPELINE - indirect draws, GPU driven
// MESH_PIPELINE - indirect mesh shading pipeline

#ifdef MESH_PIPELINE
#define VERTEX_INDEX vi
#else
#define VERTEX_INDEX gl_VertexIndex
#endif

#if !TRADITIONAL_PIPELINE && !INDIRECT_PIPELINE && !MESH_PIPELINE
	#error "No PIPELINE directive enabled"
#endif

#if TRADITIONAL_PIPELINE
	// must have push constants enabled
	#define DRAW_ID pushModel.idx
#elif INDIRECT_PIPELINE || MESH_PIPELINE
	#if CULLING_ENABLED
		#define DRAW_ID drawDataIndices[gl_DrawIDARB]
	#else
		#define DRAW_ID gl_DrawIDARB
	#endif
#endif

#if TRADITIONAL_PIPELINE || INDIRECT_PIPELINE
	#define OUT_POSITION gl_Position
#elif MESH_PIPELINE
	#define OUT_POSITION gl_MeshVerticesNV[i].gl_Position
#endif

#if TRADITIONAL_PIPELINE || INDIRECT_PIPELINE
#define OUT_DECLARATION
#define OUT
#elif MESH_PIPELINE
#define OUT_DECLARATION []
#define OUT [i]
#endif