//#include "VKBuffer.hpp"
//
//#include <vulkan/vulkan.h>
//#include <Configs/AssetsPathConfig.hpp>
//#include "VKUtils.hpp"
//#include "ForwardRenderer.hpp"
//
//using namespace Poly;
//
//VKBuffer::VKBuffer(VkDevice device, VkPhysicalDevice physicalDevice) : device(device), physicalDevice(physicalDevice)
//{
//	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMemory;
//	createBuffer(this->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory); // Create staging buffer in CPU accesible memory (With flag that buffer can be used as source in a memory transfer operation)
//
//	void* data;
//	vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data); // Map the buffer memory into CPU accessible memory with (The last parameter specifies the output for the pointer to the mapped memory)
//	memcpy(data, vertices.data(), (size_t)bufferSize); // Copy the vertex data to the buffer
//	vkUnmapMemory(this->device, stagingBufferMemory); // Unmap the buffer memory
//
//	createBuffer(this->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory); // Create vertex buffer in GPU local memory (With flag that buffer can be used as destination in a memory transfer operation)
//
//	copyBuffer(stagingBuffer, vertexBuffer, bufferSize); // Use memory transfer operation from queue family (Since vertex buffer is not accessible for CPU vkMapMemory cannot be used)
//
//	vkDestroyBuffer(this->device, stagingBuffer, nullptr);
//	vkFreeMemory(this->device, stagingBufferMemory, nullptr);
//
//}
//
//
//
//VKBuffer::~VKBuffer()
//{
//}
//
//void VKBuffer::createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
//{
//	VkBufferCreateInfo bufferInfo = {};
//	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	bufferInfo.size = size; // Size of the buffer
//	bufferInfo.usage = usage; // Indicates purpose of the data (possible to specify multiple using bitwise or)
//	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Sharing between different queue families (in this case only graphic queue so no sharing)
//
//	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
//	{
//		ASSERTE(false, "Creating buffer failed"); // Create a buffer and store a handle in vertexBuffer variable
//	}
//
//	VkMemoryRequirements memRequirements;
//	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
//
//	VkMemoryAllocateInfo allocInfo = {}; // Memory allocation parameters
//	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	allocInfo.allocationSize = memRequirements.size;
//	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties); // Find memory with flags (eg available to write for CPU)
//
//	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) // Allocate memory for buffer and store a handle in vertexBufferMemory variable
//	{
//		ASSERTE(false, "Allocating buffer memory failed");
//	}
//
//	vkBindBufferMemory(device, buffer, bufferMemory, 0); // Associate memory with the buffer (fourth parameter is the offset within the region of memory)
//}
//
//void VKBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
//{
//	//VkCommandBuffer commandBuffer = beginSingleTimeCommands();
//
//	//VkBufferCopy copyRegion = {}; // Defines the regions of buffers to copy
//	//copyRegion.srcOffset = 0; // Optional
//	//copyRegion.dstOffset = 0; // Optional
//	//copyRegion.size = size;
//	//vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion); // Transfer the contents of the buffers
//
//	//endSingleTimeCommands(commandBuffer);
//}
//
