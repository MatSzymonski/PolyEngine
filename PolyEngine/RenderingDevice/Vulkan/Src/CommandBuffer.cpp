#include "CommandBuffer.hpp"

#include <Configs/AssetsPathConfig.hpp>
#include <VKUtils.hpp>

using namespace Poly;

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocationInfo = {};
	allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocationInfo.commandPool = commandPool;
	allocationInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocationInfo, &commandBuffer); // Allocate a temporary command buffer for memory transfer operations (which just like drawing commands are executed using command buffers)

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Tell the driver that we want to use this command buffer once and wait with returning from the function until the copy operation has finished executing

	vkBeginCommandBuffer(commandBuffer, &beginInfo); // Start recording the command buffer 

	return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer); // End recording

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE); // Submit the command for execution
	vkQueueWaitIdle(queue); // Wait for transfer to complete (different way to do this is to use fences. This approach allows to schedule multiple transfer operations and wait for all of them to complete instead of executing one at a time)

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer); // Clean up the command buffer
}