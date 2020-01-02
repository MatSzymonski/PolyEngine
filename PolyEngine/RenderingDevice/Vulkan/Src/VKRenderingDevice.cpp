#include "VKRenderingDevice.hpp"
#include "PolyRenderingDeviceVKPCH.hpp"

#include <set>
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <IRendererInterface.hpp>
#include <ForwardRenderer.hpp>

#include <VKMeshDeviceProxy.hpp>
#include <CommandBuffer.hpp>
#include <Swapchain.hpp>
#include <Device.hpp>
#include <VKConfig.hpp>
#include <VKUtils.hpp>


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
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice(physicalDevice, instance, memoryProperties, surface, deviceExtensions);
	msaaSamples = getMaxUsableSampleCount(physicalDevice);
	createLogicalDevice(device, physicalDevice, surface, graphicsQueue, presentQueue, deviceExtensions, validationLayers);
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
	if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport(validationLayers))
	{
		ASSERTE(false, "Vulkan validation layers requested, but not available");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PolyEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "PolyEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions(window, instanceExtensions);
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; 
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr; 
	}

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance), "Creating Vulkan instance failed");
	core::utils::gConsole.LogInfo("Vulkan instance created successfully");
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
	core::utils::gConsole.LogError("validation layer: {}", pCallbackData->pMessage);
	//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

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
	case eRendererType::FORWARD:
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