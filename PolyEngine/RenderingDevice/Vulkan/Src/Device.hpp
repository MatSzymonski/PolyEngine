#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct SDL_Window;

void pickPhysicalDevice(VkPhysicalDevice& physicalDevice, VkInstance instance,  VkPhysicalDeviceMemoryProperties& memoryProperties, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);
VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
void createLogicalDevice(VkDevice& device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue& graphicsQueue, VkQueue& presentQueue, const std::vector<const char*> deviceExtensions, const std::vector<const char*> validationLayers);

bool checkValidationLayerSupport(const std::vector<const char*> validationLayers);
std::vector<const char*> getRequiredExtensions(SDL_Window* window, const std::vector<const char*> instanceExtensions);

