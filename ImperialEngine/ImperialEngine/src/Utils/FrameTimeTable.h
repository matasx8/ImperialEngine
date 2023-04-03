#pragma once
#include <tuple>
#include <vector>

namespace imp
{
	struct FrameTimeRow
	{
		FrameTimeRow();

		double cull;
		double draw;
		double frameMainCPU;
		double frameRenderCPU;
		double frameGPU;
	};

	struct FrameTimeTable
	{
		FrameTimeTable();

		std::vector<FrameTimeRow> table_rows;
	};
}