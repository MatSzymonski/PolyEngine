#include <Device.hpp>

#include <Configs/AssetsPathConfig.hpp>
#include <VKUtils.hpp>
#include <Swapchain.hpp>
#include <CommandBuffer.hpp>
#include <VKConfig.hpp>
#include <set>
#include <SDL_vulkan.h>

using namespace Poly;

void pickPhysicalDevice(VkPhysicalDevice& physicalDevice, VkInstance instance, VkPhysicalDeviceMemoryProperties& memoryProperties, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	ASSERTE(physicalDeviceCount > 0, "Finding GPU with Vulkan support failed"); // TODO(HIST0R)

	std::vector<VkPhysicalDevice> _physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, _physicalDevices.data());

	if (PRINT_AVAILABLE_DEVICES)
	{
		core::utils::gConsole.LogDebug("Available physical devices:");
		for (const auto& _physicalDevice : _physicalDevices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(_physicalDevice, &deviceFeatures);

			core::utils::gConsole.LogInfo(" - {}", deviceProperties.deviceName);
		}
	}

	for (const auto& _physicalDevice : _physicalDevices)
	{
		if (isDeviceSuitable(_physicalDevice, surface, deviceExtensions))
		{
			physicalDevice = _physicalDevice;
			break;
		}
	}

	ASSERTE(physicalDevice != VK_NULL_HANDLE, "Finding suitable GPU failed");

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	core::utils::gConsole.LogInfo("{} GPU selected", deviceProperties.deviceName);

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions)
{
	QueueFamilyIndices indices = findQueueFamilies(device, surface); 

	bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions); 

	bool swapchainAdequate = false;
	if (extensionsSupported) 
	{
		SwapchainSurfaceSupportDetails swapchainSurfaceSupportDetails = querySwapchainSupport(device, surface);
		swapchainAdequate = !swapchainSurfaceSupportDetails.formats.empty() && !swapchainSurfaceSupportDetails.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}


bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<core::storage::String> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}



VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void createLogicalDevice(VkDevice& device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue& graphicsQueue, VkQueue& presentQueue, const std::vector<const char*> deviceExtensions, const std::vector<const char*> validationLayers)
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = ENABLE_SIMPLE_SHADING;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());

	if (ENABLE_VALIDATION_LAYERS)
	{
		// Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but this is no longer the case
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // Depracated (ignored)
		createInfo.ppEnabledLayerNames = validationLayers.data(); // Depracated (ignored)
	}
	else
	{
		createInfo.enabledLayerCount = 0; // Depracated (ignored)
	}

	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Creating Vulkan logical device failed");

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

	core::utils::gConsole.LogInfo("Vulkan logical device successfully");
}


bool checkValidationLayerSupport(const std::vector<const char*> validationLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // Find available layers

	if (PRINT_AVAILABLE_VULKAN_LAYERS)
	{
		core::utils::gConsole.LogDebug("Available Vulkan layers:");
		for (const auto& layerProperties : availableLayers)
		{
			core::utils::gConsole.LogDebug(" - {} ({})", layerProperties.description, layerProperties.layerName);
		}
	}

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				core::utils::gConsole.LogDebug("Requested Vulkan validation layer found and loaded: {}", layerProperties.layerName);
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			core::utils::gConsole.LogDebug("Requested Vulkan validation layer not found: {}", layerName);
			return false;
		}
	}
	return true;
}

//Loads extensions needed by SDL plus extensions from list of extensions
std::vector<const char*> getRequiredExtensions(SDL_Window* window, const std::vector<const char*> instanceExtensions)
{


	unsigned int sdlExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
	std::vector<const char*> extensionNames(static_cast<uint32_t>(sdlExtensionCount));

	if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensionNames.data()))
	{
		ASSERTE(false, "Getting SDL Vulkan instance extensions failed");
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	if (PRINT_AVAILABLE_VULKAN_INSTANCE_EXTENSIONS)
	{
		core::utils::gConsole.LogDebug("Available Vulkan instance extensions:");
		for (const auto& extension : extensions)
		{
			core::utils::gConsole.LogDebug(" - {} ver. {}", extension.extensionName, extension.specVersion);
		}
	}

	for (const auto& extensionToLoad : instanceExtensions)
	{
		bool extensionFound = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(extensionToLoad, extension.extensionName) == 0)
			{
				core::utils::gConsole.LogDebug("Requested Vulkan instance extension found and loaded: {}", extensionToLoad);
				extensionNames.push_back(extensionToLoad);
				extensionFound = true;
				break;
			}
		}
		if (!extensionFound)
		{
			core::utils::gConsole.LogDebug("Requested Vulkan instance extension not found: {}", extensionToLoad);
		}
	}

	if (ENABLE_VALIDATION_LAYERS)
	{
		extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	core::utils::gConsole.LogDebug("{} Vulkan extensions loaded successfully", extensionNames.size());

	return extensionNames;
}