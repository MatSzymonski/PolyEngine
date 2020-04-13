#include "CommandBuffer.hpp"

#include <Configs/AssetsPathConfig.hpp>
#include <VKUtils.hpp>

using namespace Poly;

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) // Allocate single time command buffer and begin recording
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

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer) // Stop recording of single time command buffer, submit it to the queue and then clean it up
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


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		i++;
	}
	return indices;
}

void createCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkCommandPool& commandPool, VkCommandPoolCreateFlags flags)
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	commandPoolCreateInfo.flags = flags;

	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool), "Creating command pool failed");
}

void allocateCommandBuffers(VkDevice device, VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool &commandPool)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // (PRIMARY - Can be submitted to a queue for execution, but cannot be called from other command buffers, SECONDARY -  Cannot be submitted directly, but can be called from primary command buffers)
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer) != VK_SUCCESS)
	{
		ASSERTE(false, "Allocating command buffers failed");
	}
}