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
		friend class VKMeshDeviceProxy;
		//friend class TiledForwardRenderer;

	public:
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
		VkSurfaceKHR surface;
		VkDevice device; // Logical device
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // GPU

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkPhysicalDeviceMemoryProperties memoryProperties;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		
		IRendererInterface* renderer;

		VkCommandPool resourcesAllocationCommandPool;

		void createInstance();	
		void createSurface();

		void setupDebugMessenger();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
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