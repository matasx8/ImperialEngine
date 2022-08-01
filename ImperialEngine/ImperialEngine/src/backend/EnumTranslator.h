#pragma once
#include "frontend/EngineSettings.h"
#include <vulkan_core.h>

VkPresentModeKHR TranslatePresentMode(EnginePresentMode presentMode)
{
	switch (presentMode)
	{
	case kEnginePresentFifo:
		return VK_PRESENT_MODE_FIFO_KHR;
	case kEnginePresentMailbox:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	}
}
