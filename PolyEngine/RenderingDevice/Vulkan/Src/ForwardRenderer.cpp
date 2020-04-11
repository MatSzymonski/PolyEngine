#include "PolyRenderingDeviceVKPCH.hpp"
#include "ForwardRenderer.hpp"

#include <Configs/AssetsPathConfig.hpp>
#include "VKRenderingDevice.hpp"
#include "VKUtils.hpp"

#include <CommandBuffer.hpp>
#include <VKConfig.hpp>



//#include <vulkan/vulkan.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Poly;
using MeshQueue = core::storage::PriorityQueue<const MeshRenderingComponent*, SceneView::DistanceToCameraComparator>;

ForwardRenderer::ForwardRenderer(VKRenderingDevice * renderingDeviceInterface) 
	: IRendererInterface(renderingDeviceInterface),
	lastViewportRect(::pe::core::math::Vector2f::ZERO, core::math::Vector2f::ONE)
{
	
}

void ForwardRenderer::Init() 
{
	createSwapchain(swapchain, RDI->window, RDI->physicalDevice, RDI->device, RDI->surface, 0);
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool(RDI->device, RDI->physicalDevice, RDI->surface, commandPool);
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void ForwardRenderer::Deinit()
{
	cleanUp();	
}

void ForwardRenderer::cleanUp()
{
	cleanUpSwapchain(swapchain);

	vkDestroySampler(RDI->device, textureSampler, nullptr);

	destroyImage(textureImage, RDI->device);

	vkDestroyDescriptorSetLayout(RDI->device, descriptorSetLayout, nullptr);

	destroyBuffer(indexBuffer, RDI->device);
	destroyBuffer(vertexBuffer, RDI->device);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(RDI->device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(RDI->device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(RDI->device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(RDI->device, commandPool, nullptr);
}

void ForwardRenderer::cleanUpSwapchain(Swapchain& swapchain)
{
	destroyImage(colorTarget, RDI->device);
	destroyImage(depthTarget, RDI->device);

	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(RDI->device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(RDI->device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(RDI->device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(RDI->device, pipelineLayout, nullptr);
	vkDestroyRenderPass(RDI->device, renderPass, nullptr);

	destroySwapchain(swapchain, RDI->device);

	for (size_t i = 0; i < swapchain.imageCount; i++)
	{
		destroyBuffer(uniformBuffers[i], RDI->device);
	}

	vkDestroyDescriptorPool(RDI->device, descriptorPool, nullptr);
}

void ForwardRenderer::recreateSwapchain()
{
	core::utils::gConsole.LogInfo("Recreating swapchain");

	vkDeviceWaitIdle(RDI->device);

	Swapchain old = swapchain;
	createSwapchain(swapchain, RDI->window, RDI->physicalDevice, RDI->device, RDI->surface, old.swapchain);
	cleanUpSwapchain(old); // Wait for all the asynchronous rendering operations to finish and then close the program (to avoid errors when closing program in the middle of processing)
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void ForwardRenderer::createRenderPass() // Specify the number of color and depth buffers, number of samples for each of them and how their contents should be handled throughout the rendering operations. Also specify subpasses.
{
	VkAttachmentDescription colorAttachment = {}; // Color buffer attachment (Not for presentation since multisampled images cannot be presented directly)
	colorAttachment.format = swapchain.imageFormat; // The format of the color attachment should match the format of the swapchain images
	colorAttachment.samples = RDI->msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Determines what to do with the data in the attachment before rendering (Load/Preserve contents, clear, dont care)
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Determines what to do with the data in the attachment after rendering (Store, dont care)
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Transitions image to a required format (color attachment, present, transfer destination) 

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(RDI->physicalDevice);
	depthAttachment.samples = RDI->msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // (it will not be used after drawing has finished)
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {}; // Resolve buffer attachment (For presentation)
	colorAttachmentResolve.format = swapchain.imageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {}; // Subpass reference to the attachment in pass
	colorAttachmentRef.attachment = 0; // Attachment to refer to
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Specifies which layout the attachment will have during a subpass that uses this reference (Vulkan will automatically transition the attachment to this layout on subpass start)

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {}; // Subpass
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef; // Attachment for depth and stencil data (Unlike color attachments, a subpass can only use a single depth (+stencil) attachment)
	//subpass.pInputAttachments = ; // Attachments that are read from a shader
	subpass.pResolveAttachments = &colorAttachmentResolveRef; // Attachments used for multisampling color attachments
	//subpass.pPreserveAttachments = ; // Attachments that are not used by this subpass, but for which the data must be preserved

	VkSubpassDependency dependency = {}; // Specify the indices of the dependency and the dependent subpass - Controls image layout transitions (memory and execution dependencies between subpasses)
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Dependency subpass - Implicit subpass on rendering start (VK_SUBPASS_EXTERNAL - Implicit/hidder subpass)
	dependency.dstSubpass = 0; // Dependent subpass - Our subpass (zero index)
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Specify the operations to wait on (Wait for the swapchain to finish reading from the image before we can access it)
	dependency.srcAccessMask = 0; // Stages in which these operations occur (waiting on the color attachment output stage)
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Specify the operations to wait on
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Stages in which these operations occur
	// The operations that should wait on this are in the color attachment stage and involve the reading and writing of the color attachment. These settings will prevent the transition from happening until it's actually necessary (and allowed)

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo = {}; // Actual render pass
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(RDI->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating render pass failed");
	}
}

void ForwardRenderer::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0; // Specify the binding used in the shader
	uboLayoutBinding.descriptorCount = 1; // Specifies the number of values in the array (It is possible for the shader variable to represent an array of UBO)
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Specify type of descriptor
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional (Only relevant for image sampling related descriptors)
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Specify in which shader stages the descriptor is going to be referenced

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(RDI->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) // Create descriptor set layout
	{
		ASSERTE(false, "Creating descriptor set layout failed");
	}
}

void ForwardRenderer::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("res/Shaders/vertex.spv");
	auto fragShaderCode = readFile("res/Shaders/fragment.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule; // Specify shader module to load. (It is possible to combine multiple shaders of one type into single module and use different entry points to differentiate between their behaviors)
	vertShaderStageInfo.pName = "main"; // Specify the function to invoke (entry point)
	//vertShaderStageInfo.pSpecializationInfo = // Allows to specify values for shader constants. Shader behavior can be configured at pipeline creation by specifying different values for the constants. This is more efficient than configuring the shader using variables at render time

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; // Describes the format of the vertex data that will be passed to the vertex shader
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// Set up the graphics pipeline to accept vertex data in created "Vertex" format 
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;  // Bindings: spacing between data and whether the data is per-vertex or per-instance
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; // Describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // (Available: Point list, line list, line strip, triangle list, triangle strip)
	inputAssembly.primitiveRestartEnable = VK_FALSE; // Allows breaking up lines and triangles in the strip topology

	VkViewport viewport = {}; // Describes the region of the framebuffer that the output will be rendered to
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchain.extent.width;
	viewport.height = (float)swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {}; // Allows to discard some area of pixels in the viewport
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain.extent;

	VkPipelineViewportStateCreateInfo viewportState = {}; // Combines viewport and scissor rectangle
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; // Allows the fragments that are beyond the near and far planes to be clamped to them instead of discarded
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // Allows to disable resterizer completely
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Determines how fragments are generated for geometry (Fill, line, point)
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Determines the type of face culling to use
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Specifies the vertex order for faces to be considered front-facing and can be clockwise or counterclockwise
	rasterizer.depthBiasEnable = VK_FALSE; // Allows to alter the depth values by adding a constant value or biasing them based on a fragment's slope
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {}; // Desribes parameters of edge anti-aliasing
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = ENABLE_SIMPLE_SHADING; // enable sample shading in the pipeline
	multisampling.minSampleShading = 0.2f; // min fraction for sample shading; closer to one is smoother
	multisampling.rasterizationSamples = RDI->msaaSamples;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE; // Specify if the new depth of fragments that pass the depth test should be written to the depth buffer (Useful for drawing transparent objects)
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; //Specify the comparison that is performed to keep or discard fragments (convention of lower depth = closer)
	depthStencil.minDepthBounds = 0.0f; // Optional (Allows you to only keep fragments that fall within the specified depth range)
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	// Describes how colors of different passes are blending 
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // Contains the configuration per attached framebuffer
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {}; // Contains the global color blending settings
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; // Holds information about all uniforms in shaders
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Number of used descriptor set layouts
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Descriptor set layouts to be used by shaders
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(RDI->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating pipeline layout failed");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional (To create a new graphics pipeline by deriving from an existing pipeline - less expensive than creating new)
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(RDI->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) // Can create multiple pipelines in a single call
	{
		ASSERTE(false, "Creating graphics pipeline failed");
	}

	vkDestroyShaderModule(RDI->device, fragShaderModule, nullptr); // Destroy shader wrappers
	vkDestroyShaderModule(RDI->device, vertShaderModule, nullptr); // Destroy shader wrappers
}





void ForwardRenderer::createColorResources()
{
	createImage(colorTarget, swapchain.extent.width, swapchain.extent.height, 1, RDI->msaaSamples, swapchain.imageFormat, VK_IMAGE_TILING_OPTIMAL, RDI->device, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void ForwardRenderer::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat(RDI->physicalDevice);
	createImage(depthTarget, swapchain.extent.width, swapchain.extent.height, 1, RDI->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, RDI->device, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void ForwardRenderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapchain.imageCount);

	for (size_t i = 0; i < swapchain.imageCount; i++)
	{
		std::array<VkImageView, 3> attachments = {
			colorTarget.imageView,
			depthTarget.imageView,
			swapchain.imageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass; // Only compatible render passes can be used
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data(); // Image view and depth image view that are used by this framebuffer will be sent to the render pass so it can use them as the attachment
		framebufferInfo.width = swapchain.extent.width;
		framebufferInfo.height = swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(RDI->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			ASSERTE(false, "Creating framebuffer failed");
		}
	}
}

void ForwardRenderer::createTextureImage() //TODO REMOVE or move to resources
{
	int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load("C:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/CommonBuild/RenderingDevice/Vulkan/PolyRenderingDeviceVK/res/Textures/PolyEngine_logo.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // Load image from file
	stbi_uc* pixels = stbi_load("D:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/RenderingDevice/Vulkan/Src/res/Textures/PolyEngine_logo.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // Load image from file
	
	
	VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1; // Calculates the number of levels in the mipmap chain 

	if (!pixels)
		ASSERTE(false, "Loading texture image failed");

	Buffer stagingBuffer;
	createBuffer(stagingBuffer, RDI->device, imageSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	uploadBuffer(stagingBuffer, RDI->device, pixels);
	stbi_image_free(pixels); // Clean up original pixel array (not needed anymore)

	createImage(textureImage, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, RDI->device, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	transitionImageLayout(textureImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, RDI->device, commandPool, RDI->graphicsQueue); // Transition the image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	copyBufferToImage(stagingBuffer.buffer, textureImage.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), RDI->device, commandPool, RDI->graphicsQueue); // Execute the buffer to image copy operation																												   
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	destroyBuffer(stagingBuffer, RDI->device);
	generateMipmaps(textureImage.image, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels, RDI->device, RDI->physicalDevice, RDI->graphicsQueue, commandPool); // Generate mipmaps for this texture image

	textureSampler = createSampler(RDI->device, VK_SAMPLER_REDUCTION_MODE_MIN_EXT, mipLevels);
}



void ForwardRenderer::createVertexBuffer() //TODO REMOVE or move to resources
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	Buffer stagingBuffer;
	createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	uploadBuffer(stagingBuffer, RDI->device, vertices.data());
	createBuffer(vertexBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize, RDI->device, commandPool, RDI->graphicsQueue);
	destroyBuffer(stagingBuffer, RDI->device);
}

void ForwardRenderer::createIndexBuffer()  //TODO REMOVE or move to resources
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	Buffer stagingBuffer;
	createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	uploadBuffer(stagingBuffer, RDI->device, indices.data());
	createBuffer(indexBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	copyBuffer(stagingBuffer, indexBuffer, bufferSize, RDI->device, commandPool, RDI->graphicsQueue);
	destroyBuffer(stagingBuffer, RDI->device);
}

void ForwardRenderer::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	uniformBuffers.resize(swapchain.imageCount);

	for (size_t i = 0; i < swapchain.imageCount; i++) // For each swapchain image create uniform buffer (no staging buffer needed since uniform buffers will be updated each frame)
	{
		createBuffer(uniformBuffers[i], RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	}
}

void ForwardRenderer::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {}; // Describe which descriptor types our descriptor sets are going to contain and how many of them
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // For UBO
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain.imageCount);

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // For combined image sampler
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain.imageCount);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size()); // Maximum number of individual descriptors that are available
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapchain.imageCount); // Maximum number of descriptor sets that may be allocated
	poolInfo.flags = 0;

	if (vkCreateDescriptorPool(RDI->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating descriptor pool failed");
	}
}

void ForwardRenderer::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swapchain.imageCount, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.imageCount);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapchain.imageCount);
	if (vkAllocateDescriptorSets(RDI->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) // Allocate descriptor sets, each with one uniform buffer descriptor
	{
		ASSERTE(false, "Allocating descriptor sets failed");
	}

	for (size_t i = 0; i < swapchain.imageCount; i++) // Configure each UBO descriptor and each image sampler descriptor
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImage.imageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];// Specify the descriptor set to update
		descriptorWrites[0].dstBinding = 0; // Specify binding (In this case our uniform buffer binding index 0)
		descriptorWrites[0].dstArrayElement = 0; ; // First index in the array to update
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Specify the type of descriptor
		descriptorWrites[0].descriptorCount = 1; // Specify how many array elements to update
		descriptorWrites[0].pBufferInfo = &bufferInfo; // (used for descriptors that refer to buffer data)
		descriptorWrites[0].pImageInfo = nullptr; // (used for descriptors that refer to image data)
		descriptorWrites[0].pTexelBufferView = nullptr; // (used for descriptors that refer to buffer views)

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = nullptr; // (used for descriptors that refer to buffer data)
		descriptorWrites[1].pImageInfo = &imageInfo; // (used for descriptors that refer to image data)
		descriptorWrites[1].pTexelBufferView = nullptr; // (used for descriptors that refer to buffer views)

		vkUpdateDescriptorSets(RDI->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr); // Update descriptors
	}
}

void ForwardRenderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // (PRIMARY - Can be submitted to a queue for execution, but cannot be called from other command buffers, SECONDARY -  Cannot be submitted directly, but can be called from primary command buffers)
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(RDI->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		ASSERTE(false, "Allocating command buffers failed");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) // Command buffer recording (for every framebuffer/swapchain image)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional, relevant for secondary command buffers only

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			ASSERTE(false, "Beginning of command buffer recording failed");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain.extent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data(); // Color used for VK_ATTACHMENT_LOAD_OP_CLEAR in attachment (color and depth)

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // (INLINE - Render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed, SECONDARY_COMMAND_BUFFERS - The render pass commands will be executed from secondary command buffers)

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets); // Bind vertex buffers to bindings in shaders

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16); // Bind index buffers to bindings in shaders

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr); // Bind the right descriptor set for each swapchain image to the descriptors in the shader 

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);; // Actual draw command

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			ASSERTE(false, "Recording command buffer failed");
		}
	}
}

void ForwardRenderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT); // Set of semaphores for each frame in pool
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT); // Set of semaphores for each frame in pool
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); // Set of fences for each frame in pool
	imagesInFlight.resize(swapchain.imageCount, VK_NULL_HANDLE); // Track for each swapchain image if a frame in flight is currently using it to avoid desynchronization when there are more images than fences etc


	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Set fence state to signaled

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) // For each frame in pool create sync objects
	{
		if (vkCreateSemaphore(RDI->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(RDI->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(RDI->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			ASSERTE(false, "Creating synchronization objects for the frame failed");
		}
	}
}



















VkShaderModule ForwardRenderer::createShaderModule(const std::vector<char>& code) // Create thin wrappers around shader code. They are needed only during pipeline creation (afterwards they are not needed any more and can be destroyed)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // Size in bytes

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(RDI->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating shader module failed");
	}

	return shaderModule;
}

std::vector<char> ForwardRenderer::readFile(const std::string& filename)
{
	core::storage::StringBuilder sb;
	//sb.AppendFormat("C:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/CommonBuild/RenderingDevice/Vulkan/PolyRenderingDeviceVK/{}", filename);
	sb.AppendFormat("D:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/RenderingDevice/Vulkan/Src/{}", filename);

	core::storage::String path = sb.GetString();

	//std::cout << "Current path is " << std::filesystem::current_path() << '\n';



	std::ifstream file((std::string&)path, std::ios::ate | std::ios::binary);

	//core::utils::gConsole.LogInfo("Searching for {}", path);
	if (!file.is_open())
	{
		ASSERTE(false, "failed to open file");
	}
	else
	{
	//	core::utils::gConsole.LogInfo("oPENED {}", path);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}
















void ForwardRenderer::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1; // GLM was designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The compensate flip the sign on the scaling factor of the Y axis in the projection matrix (Without this the image will be rendered upside down)

	//void* data;
	//vkMapMemory(RDI->device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data); // Map the buffer memory into CPU accessible memory with (The last parameter specifies the output for the pointer to the mapped memory)
	//memcpy(data, &ubo, sizeof(ubo)); // Copy the data to the buffer
	//vkUnmapMemory(RDI->device, uniformBuffersMemory[currentImage]); // Unmap the buffer memory

	void* data;
	vkMapMemory(RDI->device, uniformBuffers[currentImage].memory, 0, sizeof(ubo), 0, &data); // Map the buffer memory into CPU accessible memory with (The last parameter specifies the output for the pointer to the mapped memory)
	memcpy(data, &ubo, sizeof(ubo)); // Copy the data to the buffer
	vkUnmapMemory(RDI->device, uniformBuffers[currentImage].memory); // Unmap the buffer memory
}

//----------------------- API ------------------

void ForwardRenderer::Resize(const ScreenSize& size) //TODO(HIST0R)
{
	core::utils::gConsole.LogInfo("Vulkan forward renderer resize ({}, {}), LastRect: {}", size.Width, size.Height, lastViewportRect);

	windowResized = true;
}

void ForwardRenderer::Render(const SceneView& sceneView)
{	
	vkWaitForFences(RDI->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX); // Wait for frame to be finished and then start rendering new data to it

	uint32_t imageIndex;
	VkResult result;
	result = vkAcquireNextImageKHR(RDI->device, swapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); // Start waiting for image from swapchain. When image becomes available sync object is signaled. Last parameter is index of available VkImage in swapchainImages array (using UINT64_MAX for image availability delay disables it)



	if (result == VK_ERROR_OUT_OF_DATE_KHR) // Check if swapchain is still compatible with the surface (and cannot be used for rendering any longer)
	{
		recreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		ASSERTE(false, "Acquiring swapchain image failed");
	}

	updateUniformBuffer(imageIndex);

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) // Check if a previous frame is using this image (i.e. there is its fence to wait on)
	{
		vkWaitForFences(RDI->device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	imagesInFlight[imageIndex] = inFlightFences[currentFrame]; // Mark the image as now being in use by this frame



	VkSubmitInfo submitInfo = {}; // Queue submission and synchronization
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] }; // Specifies for which semaphores to wait before execution begins
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // Specifies before which stage(s) of the pipeline to wait (Here the stage of the graphics pipeline that writes to the color attachment) (That means that theoretically the implementation can already start executing our vertex shader and such while the image is not yet available)
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex]; // Specify to which command buffer submit for (we should submit the command buffer that binds the swapchain image we just acquired as color attachment)
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] }; // To which semaphores send signal when command buffer(s) have finished execution
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(RDI->device, 1, &inFlightFences[currentFrame]); // Restore the fence to the unsignaled state (block fence)

	VkResult queueSubmitResult = vkQueueSubmit(RDI->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]); // (unblock fence after passing to render queue so it can be used for new frame render instantly)
	if (queueSubmitResult != VK_SUCCESS)
	{
		ASSERTE(queueSubmitResult == VK_ERROR_OUT_OF_HOST_MEMORY, "Submitting draw command buffer to graphics queue failed - Out of host memory");
		ASSERTE(queueSubmitResult == VK_ERROR_OUT_OF_DEVICE_MEMORY, "Submitting draw command buffer to graphics queue failed - Out of device memory");
		ASSERTE(queueSubmitResult == VK_ERROR_DEVICE_LOST, "Submitting command draw buffer to graphics queue failed - Device lost");
	}


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores; // Specify which semaphores to wait on before presentation can happen
	VkSwapchainKHR swapchains[] = { swapchain.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional Allows to specify an array of VkResult to check for every individual swapchain if presentation was successful



	result = vkQueuePresentKHR(RDI->presentQueue, &presentInfo); // Submits the request to present an image to the swapchain

	if (lastViewportRect != sceneView.Rect)
	{
		lastViewportRect = sceneView.Rect;
		Resize(RDI->GetScreenSize());
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized) // Check if swapchain is still compatible with the surface (and cannot be used for rendering any longer)
	{
		windowResized = false;
		recreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		ASSERTE(false, "Presenting swapchain image failed");
	}


	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // Advance to the next frame
}
