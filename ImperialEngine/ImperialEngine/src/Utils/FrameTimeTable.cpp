#include "FrameTimeTable.h"

imp::FrameTimeTable::FrameTimeTable()
	: table_rows()
{
}

imp::FrameTimeRow::FrameTimeRow()
	: cull(-1.0)
	, draw(-1.0)
	, frameCPU(-1.0)
	, frameGPU(-1.0)
{}
