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

void createImage(Image& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryFlags, VkImageUsageFlags usageFlags, VkImageAspectFlags imageViewAspectFlags);
void destroyImage(const Image& image, VkDevice device);

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkDevice device);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkDevice device, VkCommandPool commandPool, VkQueue queue);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDevice device, VkCommandPool commandPool, VkQueue queue);