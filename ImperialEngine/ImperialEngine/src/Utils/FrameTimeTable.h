#pragma once
#include <tuple>
#include <vector>

namespace imp
{
	struct FrameTimeRow
	{
		FrameTimeRow();

		double cull;
		double frameMainCPU;
		double frameRenderCPU;
		double frameGPU;
		double frame;
		int64_t triangles;
	};

	struct FrameTimeTable
	{
		FrameTimeTable();

		std::vector<FrameTimeRow> table_rows;
	};
}