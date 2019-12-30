#include "VKRenderingDevice.hpp"
#include "PolyRenderingDeviceVKPCH.hpp"

#include <set>
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include "IRendererInterface.hpp"
#include "ForwardRenderer.hpp"

#include "VKMeshDeviceProxy.hpp"


const bool PRINT_AVAILABLE_VULKAN_LAYERS = true;
const bool PRINT_AVAILABLE_VULKAN_INSTANCE_EXTENSIONS = true;
const bool PRINT_AVAILABLE_DEVICES = true;

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

using namespace Poly;

VKRenderingDevice* Poly::gRenderingDevice = nullptr;

IRenderingDevice* POLY_STDCALL PolyCreateRenderingDevice(SDL_Window* window, const Poly::ScreenSize& size) { return new VKRenderingDevice(window, size); }

VKRenderingDevice::VKRenderingDevice(SDL_Window* window, const ScreenSize& size)
	: window(window), screenSize(size)
{
	ASSERTE(window, "Invalid window passed to rendering device");
	ASSERTE(gRenderingDevice == nullptr, "Creating device twice?");
	gRenderingDevice = this;

	createInstance();

	rendererType = eRendererType::FORWARD; //TODO(HIST0R) Set from additional parameter, not like this

	setupDebugMessenger();
	createSurface();

	pickPhysicalDevice();
	createLogicalDevice();

	
}

VKRenderingDevice::~VKRenderingDevice()
{
	vkDeviceWaitIdle(device); // Wait for all asynchronous rendering operations to finish and then start destroying (to avoid errors when closing program in the middle of processing)

	renderer->Deinit();

	vkDestroyDevice(device, nullptr);

	if (ENABLE_VALIDATION_LAYERS)
		VKRenderingDevice::destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	//CleanUpResources(); //TODO(HIST0R) If will be any in future (like default textures etc)

	gRenderingDevice = nullptr;
}

void VKRenderingDevice::createInstance()
{
	if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
	{
		ASSERTE(false, "Vulkan validation layers requested, but not available");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PolyEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "PolyEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;


	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//if (PRINT_AVAILABLE_VULKAN_EXTENSIONS)
	//	printAvailableExtensions();

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; // Creating a separate debug utils messenger specifically for vkCreateInstance and vkDestroyInstance calls. Otherwise they will not be covered since messenger work only if instance is created
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr; // Can point to extension information in the future.
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		std::exit(-123);
		ASSERTE(false, "Creating Vulkan instance failed");
	}

	core::utils::gConsole.LogInfo("Vulkan instance created successfully");
}

bool VKRenderingDevice::checkValidationLayerSupport()
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

std::vector<const char*> VKRenderingDevice::getRequiredExtensions()
{
	unsigned int sdlExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
	std::vector<const char *> extensionNames(static_cast<uint32_t>(sdlExtensionCount));

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

	for (const auto& extensionToLoad : this->instanceExtensions)
	{
		bool extensionFound = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(extensionToLoad, extension.extensionName) == 0) 
			{
				core::utils::gConsole.LogDebug("Requested Vulkan instance extension found and loaded: {}", extensionToLoad);
				extensionNames.push_back(extension.extensionName);
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
	
	core::utils::gConsole.LogInfo("{} Vulkan extensions loaded successfully", extensionNames.size());

	return extensionNames;
}

void VKRenderingDevice::setupDebugMessenger()
{
	if (!ENABLE_VALIDATION_LAYERS) 
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		ASSERTE(false, "Setting up Vulkan debug messenger failed");
	}

	core::utils::gConsole.LogInfo("Vulkan debug messenger set up successfully");
}

void VKRenderingDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

VkResult VKRenderingDevice::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKRenderingDevice::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


VKAPI_ATTR VkBool32 VKAPI_CALL VKRenderingDevice::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	// Should always return VK_FALSE if not then there is an error with validation layer itself
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VKRenderingDevice::createSurface()
{
	if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
	{
		ASSERTE(false, "Creating SDL Vulkan surface failed!");
	}

	core::utils::gConsole.LogInfo("Vulkan surface created successfully");
}

void VKRenderingDevice::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	ASSERTE(deviceCount > 0, "Finding GPU with Vulkan support failed"); // TODO(HIST0R)

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	if (PRINT_AVAILABLE_DEVICES)
	{
		core::utils::gConsole.LogInfo("Available physical devices:");
		for (const auto& device : devices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			core::utils::gConsole.LogInfo(" - {}", deviceProperties.deviceName);
		}
	}

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			physicalDevice = device;
			msaaSamples = getMaxUsableSampleCount();

			break;
		}
	}

	ASSERTE(physicalDevice != VK_NULL_HANDLE, "Finding suitable GPU failed");

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	core::utils::gConsole.LogInfo("{} GPU selected", deviceProperties.deviceName);

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

bool VKRenderingDevice::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device); // Check if device supports requested queue families

	bool extensionsSupported = checkDeviceExtensionSupport(device); // Check if device supports requested extensions

	bool swapChainAdequate = false;
	if (extensionsSupported) // Check if swapchain fulfils additional requirements. It is sufficient if there is at least one supported image format and one supported presentation mode given the window surface used
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

VKRenderingDevice::QueueFamilyIndices VKRenderingDevice::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false; 
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		i++;
	}
	return indices;
}

bool VKRenderingDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
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

VKRenderingDevice::SwapChainSupportDetails VKRenderingDevice::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSampleCountFlagBits VKRenderingDevice::getMaxUsableSampleCount()
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

void VKRenderingDevice::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		ASSERTE(physicalDevice != VK_NULL_HANDLE, "Creating Vulkan logical device failed!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

	core::utils::gConsole.LogInfo("Vulkan logical device successfully");
}


// ---------------------------- Engine Renderer API Implementation  ----------------------------

using MeshQueue = core::storage::PriorityQueue<const MeshRenderingComponent*, SceneView::DistanceToCameraComparator>;

void VKRenderingDevice::Init()
{
	core::utils::gConsole.LogInfo("VKRenderingDevice initialization");

	//CreateUtilityTextures();

	renderer = CreateRenderer();
	renderer->Init();
}

IRendererInterface* VKRenderingDevice::CreateRenderer()
{
	IRendererInterface* renderer = nullptr;

	switch (rendererType)
	{
	case VKRenderingDevice::eRendererType::FORWARD:
		renderer = new ForwardRenderer(this);
		core::utils::gConsole.LogInfo("Creating forward Vulkan renderer");
		break;

	default:
		ASSERTE(false, "Creating renderer failed - Unknown eRenderingModeType");
	}

	return renderer;
}

void VKRenderingDevice::RenderWorld(Scene* world)
{
	for (auto&[id, viewport] : world->GetWorldComponent<ViewportWorldComponent>()->GetViewports())
	{
		SceneView sceneView(world, viewport);
		FillSceneView(sceneView);
		renderer->Render(sceneView);
	}

	// Signal frame end
	//EndFrame(); //TODO(HIST0R)
}

void VKRenderingDevice::FillSceneView(SceneView& sceneView)
{
	std::vector<const MeshRenderingComponent*> meshCmps;
	for (const auto&[meshCmp] : sceneView.SceneData->IterateComponents<MeshRenderingComponent>())
	{
		meshCmps.push_back(meshCmp);
	}

	for (const auto& meshCmp : meshCmps)
	{
		if (sceneView.CameraCmp->IsVisibleToCamera(meshCmp->GetOwner()))
		{
			if (meshCmp->GetBlendingMode() == eBlendingMode::OPAUQE)
			{
				sceneView.OpaqueQueue.Push(meshCmp);
			}
			else if (meshCmp->GetBlendingMode() == eBlendingMode::TRANSLUCENT)
			{
				sceneView.TranslucentQueue.Push(meshCmp);
			}
		}
	}

	for (const auto&[particleCmp] : sceneView.SceneData->IterateComponents<ParticleComponent>())
	{
		sceneView.ParticleQueue.Push(particleCmp);
	}

	for (const auto&[dirLightCmp] : sceneView.SceneData->IterateComponents<DirectionalLightComponent>())
	{
		sceneView.DirectionalLightList.push_back(dirLightCmp);
	}

	for (const auto&[pointLightCmp] : sceneView.SceneData->IterateComponents<PointLightComponent>())
	{
		sceneView.PointLightList.push_back(pointLightCmp);
	}

	//if (sceneView.DirectionalLightList.size() > 0)
		//CullDirLightQueue(sceneView, meshCmps);
}

void VKRenderingDevice::Resize(const ScreenSize& size)
{
	screenSize = size;

	if (renderer)
		renderer->Resize(size);
}

std::unique_ptr<ITextureDeviceProxy> VKRenderingDevice::CreateTexture(size_t width, size_t height, size_t channels, eTextureUsageType usage)
{
	return nullptr;
	//return std::make_unique<GLTextureDeviceProxy>(width, height, channels, usage);
}

std::unique_ptr<ICubemapDeviceProxy> Poly::VKRenderingDevice::CreateCubemap(size_t width, size_t height)
{
	return nullptr;
	//return std::make_unique<GLCubemapDeviceProxy>(width, height);
}

std::unique_ptr<ITextFieldBufferDeviceProxy> VKRenderingDevice::CreateTextFieldBuffer()
{
	return nullptr;
	//return std::make_unique<GLTextFieldBufferDeviceProxy>();
}

std::unique_ptr<IMeshDeviceProxy> VKRenderingDevice::CreateMesh()
{
	return nullptr;
	//return std::make_unique<VKMeshDeviceProxy>();
}

std::unique_ptr<IParticleDeviceProxy> VKRenderingDevice::CreateParticle()
{
	return nullptr;
	//return std::make_unique<GLParticleDeviceProxy>();
}