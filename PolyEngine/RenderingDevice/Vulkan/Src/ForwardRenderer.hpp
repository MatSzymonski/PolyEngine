#pragma once

#include <pe/Defines.hpp>
#include "IRendererInterface.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Resources.hpp>
#include <Swapchain.hpp>
#include <vulkan/vulkan.h>

#include <Configs/AssetsPathConfig.hpp>

#include <imgui.h>
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h" 

namespace pe::core::math
{
	class AARect;
}


namespace Poly {

	struct Frame // Frame in flight
	{
		VkFence inFlightFence;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
	};


	class ForwardRenderer : public IRendererInterface
	{
	public:

		ForwardRenderer(VKRenderingDevice* renderingDeviceInterface);
		void Init() override;
		void Resize(const ScreenSize& size) override;
		void Render(const SceneView& sceneView) override;
		void Deinit() override;

	private:
		Swapchain swapchain;
		std::vector<VkFramebuffer> swapchainFramebuffers;
		VkRenderPass renderPass;
	
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VkCommandPool commandPool;
		Image colorTarget;
		Image depthTarget;
		uint32_t mipLevels;
		Image textureImage;
		VkSampler textureSampler;

		Buffer vertexBuffer;
		Buffer indexBuffer;
		std::vector<Buffer> uniformBuffers;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkCommandBuffer> commandBuffers;

		// Frames in flight
		std::vector<Frame> frames;
		std::vector<VkFence> swapchainImagesInFlight;
		size_t currentFrameIndex = 0; // Frame index (used to use the right pair of semaphores every time)


		//ImGui
		ImGui_ImplVulkanH_Window g_MainWindowData;
		VkRenderPass imGuiRenderPass;
		VkCommandPool imGuiCommandPool;
		std::vector<VkCommandBuffer> imGuiCommandBuffers;
		std::vector<VkFramebuffer>  imGuiSwapchainFramebuffers;
		VkDescriptorPool imguiDescriptorPool;


		bool windowResized = false;
		pe::core::math::AARect lastViewportRect;
		
		void createRenderPass();
		void createDescriptorSetLayout();
		void createGraphicsPipeline();
		void createColorResources();
		void createDepthResources();
		void createFramebuffers();
		void createTextureImage();
		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorPool();
		void createDescriptorSets();
		void createCommandBuffers();
		void createFrames();

		void initializeImGui();
		void recreateImGui();
		void cleanUpImGui();

		void cleanUp();
		void cleanUpSwapchain(Swapchain& swapchain);
		void recreateSwapchain();

		VkShaderModule createShaderModule(const std::vector<char>& code);
		std::vector<char> readFile(const std::string& filename);



		void updateUniformBuffer(uint32_t currentImage);
	
	};
}
