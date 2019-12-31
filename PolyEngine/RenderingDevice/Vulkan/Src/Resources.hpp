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
void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue queue);
void destroyBuffer(const Buffer& buffer, VkDevice device);



struct Image
{
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
};

void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);