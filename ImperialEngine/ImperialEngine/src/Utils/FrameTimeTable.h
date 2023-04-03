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
		double frameCPU;
		double frameGPU;
	};

	struct FrameTimeTable
	{
		FrameTimeTable();

		std::vector<FrameTimeRow> table_rows;
	};
}