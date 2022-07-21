#pragma once

namespace imp
{
	class GraphicsCaps
	{
	public:
		GraphicsCaps();

		bool ValidationLayersSupported();

		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	private:


	};
}