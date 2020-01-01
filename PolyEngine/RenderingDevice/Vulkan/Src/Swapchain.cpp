#include <Swapchain.hpp>

#include <Configs/AssetsPathConfig.hpp>
#include <VKUtils.hpp>
#include <CommandBuffer.hpp>
#include <Resources.hpp>

#define VSYNC 1

using namespace Poly;

//VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window)
//{
//#if defined(VK_USE_PLATFORM_WIN32_KHR)
//	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
//	createInfo.hinstance = GetModuleHandle(0);
//	createInfo.hwnd = glfwGetWin32Window(window);
//
//	VkSurfaceKHR surface = 0;
//	VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
//	return surface;
//#else
//#error Unsupported platform
//#endif
//}

SwapchainSurfaceSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapchainSurfaceSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

//TODO Check how image view were created and why they are not now!
void createSwapchain(Swapchain& swapchain, SDL_Window* window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR oldSwapchain)
{
	SwapchainSurfaceSupportDetails swapchainSurfaceSupportDetails = querySwapchainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSurfaceSupportDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSurfaceSupportDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapchainSurfaceSupportDetails.capabilities, window);

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount =  std::max(2u, swapchainSurfaceSupportDetails.capabilities.minImageCount);
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; 
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndicesValues[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; 
		createInfo.queueFamilyIndexCount = 2; 
		createInfo.pQueueFamilyIndices = queueFamilyIndicesValues; 
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr; 
	}

	createInfo.preTransform = swapchainSurfaceSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain; 

	VkSwapchainKHR vkSwapchain = 0;
	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &vkSwapchain), "Creating swapchain failed");

	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device, vkSwapchain, &imageCount, nullptr), "Query the final number of images in swapchain failed");

	std::vector<VkImage> images(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device, vkSwapchain, &imageCount, images.data()), "Getting swapchain images failed");

	std::vector<VkImageView> imageViews(imageCount);
	for (size_t i = 0; i < imageViews.size(); i++)
	{
		imageViews[i] = createImageView(images[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, device);
	}

	swapchain.swapchain = vkSwapchain;
	swapchain.imageFormat = surfaceFormat.format;
	swapchain.extent = extent;
	swapchain.images = images;
	swapchain.imageViews = imageViews;
	swapchain.imageCount = imageCount;
}





//VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
//{
//	uint32_t formatCount = 0;
//	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, 0));
//	assert(formatCount > 0);
//
//	std::vector<VkSurfaceFormatKHR> formats(formatCount);
//	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));
//
//	if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
//		return VK_FORMAT_R8G8B8A8_UNORM;
//
//	for (uint32_t i = 0; i < formatCount; ++i)
//		if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
//			return formats[i].format;
//
//	return formats[0].format;
//}

void destroySwapchain(const Swapchain& swapchain, VkDevice device)
{
	for (size_t i = 0; i < swapchain.imageCount; i++)
	{
		vkDestroyImageView(device, swapchain.imageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}
//
//SwapchainStatus updateSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format)
//{
//	VkSurfaceCapabilitiesKHR surfaceCaps;
//	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));
//
//	uint32_t newWidth = surfaceCaps.currentExtent.width;
//	uint32_t newHeight = surfaceCaps.currentExtent.height;
//
//	if (newWidth == 0 || newHeight == 0)
//		return Swapchain_NotReady;
//
//	if (result.width == newWidth && result.height == newHeight)
//		return Swapchain_Ready;
//
//	Swapchain old = result;
//
//	createSwapchain(result, physicalDevice, device, surface, familyIndex, format, old.swapchain);
//
//	VK_CHECK(vkDeviceWaitIdle(device));
//
//	destroySwapchain(device, old);
//
//	return Swapchain_Resized;
//}






VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // Choose the best available mode
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) // Choose the best available mode (Mailbox is triple buffering)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; //Guaranteed to be available
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else // Picks the resolution that best matches the window within the minImageExtent and maxImageExtent bounds
	{
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		//glfwGetFramebufferSize(RDI->window, &width, &height); // Query the current size of the framebuffer to make sure that the swapchain images have the (new) right size

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

