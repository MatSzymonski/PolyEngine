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

void createSwapchain(Swapchain& swapchain, SDL_Window* window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t desiredImageCount, VkSwapchainKHR oldSwapchain)
{
	SwapchainSurfaceSupportDetails swapchainSurfaceSupportDetails = querySwapchainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSurfaceSupportDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSurfaceSupportDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapchainSurfaceSupportDetails.capabilities, window);

	uint32_t imageCount = desiredImageCount;
	if (swapchainSurfaceSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapchainSurfaceSupportDetails.capabilities.maxImageCount) 
	{
		imageCount = swapchainSurfaceSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndicesValues[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesValues;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainCreateInfo.preTransform = swapchainSurfaceSupportDetails.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR vkSwapchain = 0;
	VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &vkSwapchain), "Creating swapchain failed");


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


void destroySwapchain(const Swapchain& swapchain, VkDevice device)
{
	for (size_t i = 0; i < swapchain.imageCount; i++)
	{
		vkDestroyImageView(device, swapchain.imageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}


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

