#pragma once

namespace Poly {

	const int MAX_FRAMES_IN_FLIGHT = 2; // Maximum number of frames that can be processed concurrently
	const int COMMAND_BUFFERS_PER_FRAME = 1;
	const bool ENABLE_SIMPLE_SHADING = false;
	const bool PRINT_AVAILABLE_VULKAN_LAYERS = true;
	const bool PRINT_AVAILABLE_VULKAN_INSTANCE_EXTENSIONS = true;
	const bool PRINT_AVAILABLE_DEVICES = true;
	const bool USE_IMGUI = false;

#ifdef NDEBUG
	const bool ENABLE_VALIDATION_LAYERS = false;
#else
	const bool ENABLE_VALIDATION_LAYERS = true;
#endif

	enum class eRendererType
	{
		FORWARD
	};

	const eRendererType rendererType = eRendererType::FORWARD;

	const std::vector<const char*> validationLayers = { // Validation layers required (to load)
			"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> instanceExtensions = { // Extensions required (to load) 
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME

	};

}


