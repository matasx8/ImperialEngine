#include "FrameTimeTable.h"

imp::FrameTimeTable::FrameTimeTable()
	: table_rows()
{
}

imp::FrameTimeRow::FrameTimeRow()
	: cull(-1.0)
	, frameMainCPU(-1.0)
	, frameRenderCPU(-1.0)
	, frameGPU(-1.0)
	, frame(-1.0)
	, triangles(-1)
{}
