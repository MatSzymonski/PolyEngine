#include "VKBuffer.hpp"

#include <Configs/AssetsPathConfig.hpp>
#include <VKUtils.hpp>
#include <CommandBuffer.hpp>

using namespace Poly;

void createBuffer(Buffer& buffer, VkDevice device, VkDeviceSize size, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryFlags, VkBufferUsageFlags usageFlags)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usageFlags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer vkBuffer = 0;
	VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &vkBuffer), "Creating buffer failed");

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, vkBuffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags); // Find memory with flags (eg available to write for CPU)

	VkDeviceMemory memory = 0;
	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &memory), "Allocating buffer memory failed");  // Allocate memory for buffer and store a handle in memory variable

	VK_CHECK(vkBindBufferMemory(device, vkBuffer, memory, 0), "Binding buffer memory failed"); // Associate memory with the buffer (fourth parameter is the offset within the region of memory)


	//To remove if buffers will work
	/*void* data = 0;
	if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		VK_CHECK(vkMapMemory(device, memory, 0, size, 0, &data));*/


	buffer.buffer = vkBuffer;
	buffer.memory = memory;
	buffer.size = size;
	buffer.memoryFlags = memoryFlags;
	buffer.data = 0;
	
}

void uploadBuffer(Buffer & buffer, VkDevice device, void* data)
{
	if (!(buffer.memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		ASSERTE(false, "Uploading to buffer failed. Buffer is not host visible");

	vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &data);
	memcpy(buffer.data, data, buffer.size);
	vkUnmapMemory(device, buffer.memory);
}

void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion = {}; // Defines the regions of buffers to copy
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion); // Transfer the contents of the buffers

	//TODO(HIST0R) Maybe use this?
	//VkBufferMemoryBarrier copyBarrier = bufferBarrier(buffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &copyBarrier, 0, 0);

	endSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

void destroyBuffer(const Buffer& buffer, VkDevice device)
{
	vkDestroyBuffer(device, buffer.buffer, 0);
	vkFreeMemory(device, buffer.memory, 0);
}
