#pragma once

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <vector>
//struct SDL_Window;


	struct Swapchain
	{
		VkSwapchainKHR swapchain;
		VkFormat imageFormat;
		VkExtent2D extent;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
		uint32_t imageCount;
	};

	struct SwapchainSurfaceSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	typedef struct GLFWwindow GLFWwindow;

	VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
	VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	void createSwapchain(Swapchain& swapchain, SDL_Window* window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR oldSwapchain = 0);
	void destroySwapchain(const Swapchain& swapchain, VkDevice device);

	SwapchainSurfaceSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);

	enum SwapchainStatus
	{
		Swapchain_Ready,
		Swapchain_Resized,
		Swapchain_NotReady,
	};

	SwapchainStatus updateSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format);
