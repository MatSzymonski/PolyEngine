#pragma once

#include <pe/Defines.hpp>
#include "IRendererInterface.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Resources.hpp>
#include <Swapchain.hpp>
#include <vulkan/vulkan.h>

#include <Configs/AssetsPathConfig.hpp>

namespace pe::core::math
{
	class AARect;
}


namespace Poly {

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

		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkRenderPass renderPass;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkCommandPool commandPool; // Manage the memory that is used to store the buffers

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

		std::vector<VkSemaphore> imageAvailableSemaphores; // Set of semaphores for each frame in pool
		std::vector<VkSemaphore> renderFinishedSemaphores; // Set of semaphores for each frame in pool
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0; // Frame index (used to use the right pair of semaphores every time)

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
		void createSyncObjects();

		void cleanUp();
		void cleanUpSwapchain(Swapchain& swapchain);
		void recreateSwapchain();

		VkShaderModule createShaderModule(const std::vector<char>& code);
		std::vector<char> readFile(const std::string& filename);



		void updateUniformBuffer(uint32_t currentImage);
	
	};
}
