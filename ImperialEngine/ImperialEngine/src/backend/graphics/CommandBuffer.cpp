#include "CommandBuffer.h"
#include <cassert>
#include <stdio.h>

inline constexpr VkCommandBufferBeginInfo kBeginInfo = 
{ 
	VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
	nullptr, 
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
	nullptr 
};

imp::CommandBuffer::CommandBuffer()
	: cmb(), m_CurrentStage(kCBStageNew)
{
}

imp::CommandBuffer::CommandBuffer(VkCommandBuffer cb)
	: cmb(cb), m_CurrentStage(kCBStageNew)
{
}

void imp::CommandBuffer::Begin()
{
	const auto res = vkBeginCommandBuffer(cmb, &kBeginInfo);
	assert(res == VK_SUCCESS);

	assert(m_CurrentStage == kCBStageNew);
	m_CurrentStage = kCBStageActive;
}

imp::CommandBufferStage imp::CommandBuffer::GetCurrentStage() const
{
	return static_cast<CommandBufferStage>(m_CurrentStage);
}

void imp::CommandBuffer::End()
{
	const auto res = vkEndCommandBuffer(cmb);
	assert(res == VK_SUCCESS);

	assert(m_CurrentStage == kCBStageActive);
	m_CurrentStage = kCBStageDone;
}

void imp::CommandBuffer::ResetStageToNew()
{
	m_CurrentStage = kCBStageNew;
}
