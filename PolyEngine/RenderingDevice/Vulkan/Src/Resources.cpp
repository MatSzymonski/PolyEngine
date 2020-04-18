#include <Resources.hpp>

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
	allocateInfo.memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags); 

	VkDeviceMemory memory = 0;
	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &memory), "Allocating buffer memory failed");  

	VK_CHECK(vkBindBufferMemory(device, vkBuffer, memory, 0), "Binding buffer memory failed"); 

	buffer.buffer = vkBuffer;
	buffer.memory = memory;
	buffer.size = size;
	buffer.memoryFlags = memoryFlags;
	buffer.data = 0;
	
}
//void uploadBuffer(Buffer & buffer, VkDevice device, void* data)
void uploadBuffer(Buffer& buffer, VkDevice device, const void* data)
{
	if (!(buffer.memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		ASSERTE(false, "Uploading to buffer failed. Buffer is not host visible");

	/*vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &data);
	memcpy(buffer.data, data, buffer.size);
	vkUnmapMemory(device, buffer.memory);*/

	void* mapData = 0;
	vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &mapData);
	buffer.data = mapData;


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
	//delete &buffer; //TODO(HIST0R) Is it really needed?
}

void createImage(Image& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryFlags, VkImageUsageFlags usageFlags, VkImageAspectFlags imageViewAspectFlags)
{
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usageFlags;
	createInfo.samples = numSamples;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.flags = 0;

	VkImage vkImage = 0;
	VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &vkImage), "Creating image failed");

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, vkImage, &memoryRequirements);

	VkMemoryAllocateInfo allocationInfo = {};
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.allocationSize = memoryRequirements.size;
	allocationInfo.memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);

	VkDeviceMemory memory = 0;
	VK_CHECK(vkAllocateMemory(device, &allocationInfo, nullptr, &memory), "Allocating image memory failed");

	vkBindImageMemory(device, vkImage, memory, 0);

	image.image = vkImage;
	image.imageView = createImageView(vkImage, format, imageViewAspectFlags, mipLevels, device);
	image.memory = memory;

}

void destroyImage(const Image& image, VkDevice device)
{
	vkDestroyImageView(device, image.imageView, 0);
	vkDestroyImage(device, image.image, 0);
	vkFreeMemory(device, image.memory, 0); 
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkDevice device)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; 
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectFlags; 
	createInfo.subresourceRange.baseMipLevel = 0; 
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView = 0;
	VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &imageView), "Creating image view failed");

	return imageView;
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkImageMemoryBarrier barrier = {}; // Create image memory barrier to synchronize access to image and transition image layouts  
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout; // Specify layout transition
	barrier.newLayout = newLayout; // Specify layout transition
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For using barrier to transfer queue family ownership
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For using barrier to transfer queue family ownership
	barrier.image = image; // Specify the image that is affected 
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // subresourceRange specify the specific part of the image
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) // Undefined -> transfer destination transition (don't need to wait on anything)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // The earliest possible pipeline stage (is not a real stage, It is a pseudo-stage where transfers happen)
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // Transfer destination -> shader transition (Shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where texture is used)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // Transition should occur in the pipeline transfer stage, since writes don't have to wait on anything)
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Barrier before fragmen shader (since it can read from unfinished image)
	}
	else
	{
		ASSERTE(false, "Transitioning image layout failed - Unsupported layout transition");
	}

	vkCmdPipelineBarrier( // Submit pipeline barrier
		commandBuffer,
		sourceStage, // Specify the pipeline stage in which operations occur that should happen before the barrier
		destinationStage, // Specify the pipeline stage in which operations will wait on the barrier
		0, // (Whether turn the barrier into a per-region condition - That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far)
		0, nullptr, // Specify what to create (memory barriers)
		0, nullptr, // Specify what to create (buffer memory barriers)
		1, &barrier // Specify what to create (image memory barriers)
	);

	endSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDevice device, VkCommandPool commandPool, VkQueue queue) // Specify which part of the buffer is going to be copied to which part of the image and copy them
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkBufferImageCopy region = {}; // Defines the regions of buffers to copy
	region.bufferOffset = 0; // Specify the byte offset in the buffer at which the pixel values start
	region.bufferRowLength = 0; // Specify how the pixels are laid out in memory 
	region.bufferImageHeight = 0; // Specify how the pixels are laid out in memory (padding bytes between rows)
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { // Specify to which part of the image we want to copy the pixels
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region); // Transfer the contents of the buffer to image (The fourth parameter indicates which layout the image is currently using)

	endSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

VkSampler createSampler(VkDevice device, VkSamplerReductionModeEXT reductionMode, uint32_t mipLevels)
{
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = 16;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = static_cast<float>(mipLevels);

	VkSamplerReductionModeCreateInfoEXT createInfoReduction = {};
	createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;

	if (reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT)
	{
		createInfoReduction.reductionMode = reductionMode;
		createInfo.pNext = &createInfoReduction;
	}

	VkSampler sampler = 0;
	VK_CHECK(vkCreateSampler(device, &createInfo, nullptr, &sampler), "Creating sampler failed");
	return sampler;
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	ASSERTE(false, "Finding supported depth image format failed");
	return candidates[0]; //TODO(HIST0R) what to return? Unreachable code btw
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physicalDevice);
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
{
	// Each level will be transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the blit command reading from it is finished

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) // Check if image format supports linear blitting (since support is not guaranteed on all platforms)
	{
		ASSERTE(false, "Generating mipmaps failed - Texture image format does not support linear blitting");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) // Record VkCmdBlitImage commands generating mmipmaps (Each time generate mipmap one level higher)
	{
		// First, we transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. 
		// This transition will wait for level i - 1 to be filled, either from the previous blit command, or from vkCmdCopyBufferToImage. The current blit command will wait on this transition.

		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 }; // Determine the 3D region that data will be blitted from
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 }; // Determine the 3D region that data will be blitted from
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 }; // Determines the region that data will be blitted to
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }; // Determines the region that data will be blitted to (divided by two since each mip level is half the size of the previous level)
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, // Record blit command
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Transition from
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Transition to
			1, &blit,
			VK_FILTER_LINEAR); // Specify filtering 

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, // This transition waits on the current blit command to finish (All sampling operations will wait on this transition to finish)
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2; // Divide the current mip dimensions by two
		if (mipHeight > 1) mipHeight /= 2; // Divide the current mip dimensions by two
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, // Wait for transition the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}