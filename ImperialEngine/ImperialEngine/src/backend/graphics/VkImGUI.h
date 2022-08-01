#pragma once
#include "Utils/NonCopyable.h"
#include <imgui.h>
#include <vulkan.h>

namespace imp
{
	// backend implementation for dear imgui
	class VkImGUI : NonCopyable
	{
	public:
		VkImGUI();
		void Initialize(VkDevice device);

		void Destroy();
	private:

	};
}