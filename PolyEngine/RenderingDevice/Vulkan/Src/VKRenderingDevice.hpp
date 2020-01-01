#pragma once

#include <pe/Defines.hpp>
#include <Rendering/IRenderingDevice.hpp>
#include <pe/core/storage/String.hpp>

//#include "PolyRenderingDeviceVKPCH.hpp"
//
#include <vulkan/vulkan.h>
//#include <SDL.h>
//#include <SDL_vulkan.h>

struct SDL_Window;

namespace pe::core::math
{
	class AARect;
}

namespace Poly
{
	struct SceneView;
	class CameraComponent;
	class Scene;
	class RenderingPassBase;
	class RenderingTargetBase;
	class IRendererInterface;

	// Holds things that are common for many renderers
	class DEVICE_DLLEXPORT VKRenderingDevice : public IRenderingDevice
	{
		friend class ForwardRenderer;
		//friend class TiledForwardRenderer;

	private:

		enum class eRendererType
		{
			FORWARD
		};




	public:

		const int MAX_FRAMES_IN_FLIGHT = 2; // Maximum number of frames that can be processed concurrently
		const bool ENABLE_SIMPLE_SHADING = false;

		VKRenderingDevice(SDL_Window* window, const Poly::ScreenSize& size);
		~VKRenderingDevice();

		VKRenderingDevice(const VKRenderingDevice&) = delete;
		void operator=(const VKRenderingDevice&) = delete;

		void Init() override;
		void Resize(const ScreenSize& size) override;
		void RenderWorld(Scene* world) override;
		const ScreenSize& GetScreenSize() const override { return screenSize; }

	private:

		SDL_Window* window;
		ScreenSize screenSize;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // GPU
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkDevice device; // Logical device

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		IRendererInterface* renderer;
		eRendererType rendererType;

		VkPhysicalDeviceMemoryProperties memoryProperties;
		

		const std::vector<const char*> validationLayers = { // Validation layers required (to load)
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> instanceExtensions = { // Extensions required (to load) //TODO(HIST0R) throws error when any
			//"VK_EXT_debug_report" //TODO(HIST0R) How to actually add extensions?
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};


		void createInstance();
		bool checkValidationLayerSupport();
		std::vector<const char*> getRequiredExtensions();
		void setupDebugMessenger();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
		void createSurface();
		void pickPhysicalDevice();
		bool isDeviceSuitable(VkPhysicalDevice device);

		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		VkSampleCountFlagBits getMaxUsableSampleCount();
		void createLogicalDevice();

		void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);




		// Engine Renderer API 
		IRendererInterface* CreateRenderer();
		void FillSceneView(SceneView& sceneView);		
		std::unique_ptr<ITextureDeviceProxy> CreateTexture(size_t width, size_t height, size_t channels, eTextureUsageType usage) override;
		std::unique_ptr<ICubemapDeviceProxy> CreateCubemap(size_t width, size_t height) override;
		std::unique_ptr<ITextFieldBufferDeviceProxy> CreateTextFieldBuffer() override;
		std::unique_ptr<IMeshDeviceProxy> CreateMesh() override;
		std::unique_ptr<IParticleDeviceProxy> CreateParticle() override;
	};

	extern VKRenderingDevice* gRenderingDevice;
}

extern "C"
{
	DEVICE_DLLEXPORT Poly::IRenderingDevice* POLY_STDCALL PolyCreateRenderingDevice(SDL_Window* window, const Poly::ScreenSize& size);
}