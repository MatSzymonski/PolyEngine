#pragma once

#include <pe/Defines.hpp>
#include "IRendererInterface.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Resources.hpp>

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
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkRenderPass renderPass;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkCommandPool commandPool; // Manage the memory that is used to store the buffers

		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		uint32_t mipLevels;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;

		//VkBuffer vertexBuffer;
		//VkDeviceMemory vertexBufferMemory;
		Buffer vertexBuffer;
		Buffer indexBuffer;

		//VkBuffer indexBuffer;
		//VkDeviceMemory indexBufferMemory;
		std::vector<Buffer> uniformBuffers;
		//std::vector<VkDeviceMemory> uniformBuffersMemory;

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
		

		void drawFrame();

		void createSwapChain();
		void createImageViews();
		void createRenderPass();
		void createDescriptorSetLayout();
		void createGraphicsPipeline();
		void createCommandPool();
		void createColorResources();
		void createDepthResources();
		void createFramebuffers();
		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();
		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorPool();
		void createDescriptorSets();
		void createCommandBuffers();
		void createSyncObjects();

		void cleanUp();
		void cleanUpSwapChain();
		void recreateSwapChain();

		VkShaderModule createShaderModule(const std::vector<char>& code);
		std::vector<char> readFile(const std::string& filename);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat findDepthFormat();
		bool hasStencilComponent(VkFormat format);
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		
		void updateUniformBuffer(uint32_t currentImage);
	
	};
}
