#pragma once

#include <vulkan/vulkan.h>


struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
	VkMemoryPropertyFlags memoryFlags;
	void* data;
};

void createBuffer(Buffer& buffer, VkDevice device, VkDeviceSize size, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags propertyFlags, VkBufferUsageFlags usageFlags);
void uploadBuffer(Buffer& buffer, VkDevice device, const void* data);
void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size);
void destroyBuffer(const Buffer& buffer, VkDevice device);
