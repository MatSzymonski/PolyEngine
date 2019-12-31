#include "PolyRenderingDeviceVKPCH.hpp"
#include "ForwardRenderer.hpp"

#include <Configs/AssetsPathConfig.hpp>
#include "VKRenderingDevice.hpp"
#include "VKUtils.hpp"

#include <CommandBuffer.hpp>

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
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
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
	cleanUpSwapChain();

	vkDestroySampler(RDI->device, textureSampler, nullptr);
	vkDestroyImageView(RDI->device, textureImageView, nullptr);

	vkDestroyImage(RDI->device, textureImage, nullptr);
	vkFreeMemory(RDI->device, textureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(RDI->device, descriptorSetLayout, nullptr);

	destroyBuffer(indexBuffer, RDI->device);
	//vkDestroyBuffer(RDI->device, indexBuffer, nullptr);
	//vkFreeMemory(RDI->device, indexBufferMemory, nullptr);

	destroyBuffer(vertexBuffer, RDI->device);

	//vkDestroyBuffer(RDI->device, vertexBuffer, nullptr);
	//vkFreeMemory(RDI->device, vertexBufferMemory, nullptr);

	for (size_t i = 0; i < RDI->MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(RDI->device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(RDI->device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(RDI->device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(RDI->device, commandPool, nullptr);
}

void ForwardRenderer::cleanUpSwapChain()
{
	vkDestroyImageView(RDI->device, colorImageView, nullptr);
	vkDestroyImage(RDI->device, colorImage, nullptr);
	vkFreeMemory(RDI->device, colorImageMemory, nullptr);

	vkDestroyImageView(RDI->device, depthImageView, nullptr);
	vkDestroyImage(RDI->device, depthImage, nullptr);
	vkFreeMemory(RDI->device, depthImageMemory, nullptr);

	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(RDI->device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(RDI->device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(RDI->device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(RDI->device, pipelineLayout, nullptr);
	vkDestroyRenderPass(RDI->device, renderPass, nullptr);

	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(RDI->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(RDI->device, swapChain, nullptr);

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		destroyBuffer(uniformBuffers[i], RDI->device);

	//	vkDestroyBuffer(RDI->device, uniformBuffers[i], nullptr);
	//	vkFreeMemory(RDI->device, uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(RDI->device, descriptorPool, nullptr);
}

void ForwardRenderer::recreateSwapChain()
{
	core::utils::gConsole.LogInfo("Recreating swapchain");

	vkDeviceWaitIdle(RDI->device);

	cleanUpSwapChain(); // Wait for all the asynchronous rendering operations to finish and then close the program (to avoid errors when closing program in the middle of processing)

	createSwapChain();
	createImageViews();
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

void ForwardRenderer::createSwapChain()
{
	VKRenderingDevice::SwapChainSupportDetails swapChainSupport = RDI->querySwapChainSupport(RDI->physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;// Set the number of images in swap chain
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = RDI->surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // Number of layers per image (always 1 for 2D usage)
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to swapchain images (If rendering to some other buffer, using postprocessing and then transfering to swapchain - use VK_IMAGE_USAGE_TRANSFER_DST_BIT)

	VKRenderingDevice::QueueFamilyIndices indices = RDI->findQueueFamilies(RDI->physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	// Graphics and presentation queue families are the same, which is the case on most hardware 
	// Specify how to handle swap chain images that will be used across multiple queue families
	if (indices.graphicsFamily != indices.presentFamily) // Draw first on the images in the swapchain from the graphics queue and then submit them on the presentation queue
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Images can be used across multiple queue families without explicit ownership transfers
		createInfo.queueFamilyIndexCount = 2; // Specify in advance between which queue families ownership will be shared
		createInfo.pQueueFamilyIndices = queueFamilyIndices; // Specify in advance between which queue families ownership will be shared
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //  Image is owned by one queue family at a time and ownership must be explicitly transfered before using it in another queue family. This option offers the best performance.
		createInfo.queueFamilyIndexCount = 0; // Specify in advance between which queue families ownership will be shared
		createInfo.pQueueFamilyIndices = nullptr; // Specify in advance between which queue families ownership will be shared
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // Additional image transform in swapchain (like rotating by 90 deg)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // If the alpha channel should be used for blending with other windows in the window system (Ignore in this case)
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; // Don't care about the color of pixels that are obscured

	createInfo.oldSwapchain = VK_NULL_HANDLE; // Reference to the old not anymore valid swapchain (New one needs to be created eg. when resizing window)

	if (vkCreateSwapchainKHR(RDI->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating swapchain failed");
	}

	vkGetSwapchainImagesKHR(RDI->device, swapChain, &imageCount, nullptr); // Query the final number of images in swapchain
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(RDI->device, swapChain, &imageCount, swapChainImages.data()); // Get image handles

	swapChainImageFormat = surfaceFormat.format; // Store the format for the swapchain images in member variables for later use
	swapChainExtent = extent; // Store the extent for the swapchain images in member variables for later use
}

void ForwardRenderer::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size()); // Resize the list to fit all of the image views

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void ForwardRenderer::createRenderPass() // Specify the number of color and depth buffers, number of samples for each of them and how their contents should be handled throughout the rendering operations. Also specify subpasses.
{
	VkAttachmentDescription colorAttachment = {}; // Color buffer attachment (Not for presentation since multisampled images cannot be presented directly)
	colorAttachment.format = swapChainImageFormat; // The format of the color attachment should match the format of the swapchain images
	colorAttachment.samples = RDI->msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Determines what to do with the data in the attachment before rendering (Load/Preserve contents, clear, dont care)
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Determines what to do with the data in the attachment after rendering (Store, dont care)
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Transitions image to a required format (color attachment, present, transfer destination) 

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = RDI->msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // (it will not be used after drawing has finished)
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {}; // Resolve buffer attachment (For presentation)
	colorAttachmentResolve.format = swapChainImageFormat;
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
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {}; // Allows to discard some area of pixels in the viewport
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

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
	multisampling.sampleShadingEnable = RDI->ENABLE_SIMPLE_SHADING; // enable sample shading in the pipeline
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



void ForwardRenderer::createCommandPool()
{
	VKRenderingDevice::QueueFamilyIndices queueFamilyIndices = RDI->findQueueFamilies(RDI->physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(RDI->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating command pool failed");
	}
}

void ForwardRenderer::createColorResources()
{
	VkFormat colorFormat = swapChainImageFormat;

	// In case of images with more than one sample per pixel only one mipmap level can be used
	createImage(swapChainExtent.width, swapChainExtent.height, 1, RDI->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
	colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void ForwardRenderer::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat(); // Find best supported format for the depth image

	createImage(swapChainExtent.width, swapChainExtent.height, 1, RDI->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void ForwardRenderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 3> attachments = {
			colorImageView,
			depthImageView,
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass; // Only compatible render passes can be used
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data(); // Image view and depth image view that are used by this framebuffer will be sent to the render pass so it can use them as the attachment
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(RDI->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			ASSERTE(false, "Creating framebuffer failed");
		}
	}
}

void ForwardRenderer::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load("C:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/CommonBuild/RenderingDevice/Vulkan/PolyRenderingDeviceVK/res/Textures/PolyEngine_logo.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // Load image from file
	stbi_uc* pixels = stbi_load("D:/GameDevelopment/PolyEngine/Engine/PolyEngine/PolyEngine/RenderingDevice/Vulkan/Src/res/Textures/PolyEngine_logo.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // Load image from file
	VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1; // Calculates the number of levels in the mipmap chain 

	if (!pixels)
	{
		ASSERTE(false, "Loading texture image failed");
	}

	Buffer stagingBuffer;
	createBuffer(stagingBuffer, RDI->device, imageSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	uploadBuffer(stagingBuffer, RDI->device, pixels);
	stbi_image_free(pixels); // Clean up original pixel array (not needed anymore)
	createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory); // Create actual image (to be stored in GPU)
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels); // Transition the image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	copyBufferToImage(stagingBuffer.buffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)); // Execute the buffer to image copy operation																												   
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	destroyBuffer(stagingBuffer, RDI->device);
	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels); // Generate mipmaps for this texture image
}

void ForwardRenderer::createTextureImageView()
{
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void ForwardRenderer::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR; // Specify how to interpolate texels that are magnified or minified (to deal with the oversampling and undersampling problems)
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // Image edge transformations (repeat, clamp to edge, clamp to border)
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE; // Specify if anisotropic filtering should be used
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Specify which color is returned when sampling beyond the image with clamp to border addressing mode
	samplerInfo.unnormalizedCoordinates = VK_FALSE; // Specify which coordinate system will be used to address texels in an image (VK_TRUE - [0, texWidth) and [0, texHeight), VK_FALSE - [0, 1) and [0, 1))
	samplerInfo.compareEnable = VK_FALSE; // If enabled texels will first be compared to a value and the result is used in filtering operations (Used for percentage-closer filtering on shadow maps)
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Mipmapping
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipLevels); // Maximal mipmap level (higher level = lower quality)

	if (vkCreateSampler(RDI->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating texture sampler failed");
	}
}

void ForwardRenderer::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	Buffer stagingBuffer;
	createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	uploadBuffer(stagingBuffer, RDI->device, vertices.data());
	createBuffer(vertexBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize, RDI->device, commandPool, RDI->graphicsQueue);
	destroyBuffer(stagingBuffer, RDI->device);
}

void ForwardRenderer::createIndexBuffer()
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
	uniformBuffers.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) // For each swapchain image create uniform buffer (no staging buffer needed since uniform buffers will be updated each frame)
	{
		createBuffer(uniformBuffers[i], RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	}
}

void ForwardRenderer::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {}; // Describe which descriptor types our descriptor sets are going to contain and how many of them
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // For UBO
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // For combined image sampler
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size()); // Maximum number of individual descriptors that are available
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size()); // Maximum number of descriptor sets that may be allocated
	poolInfo.flags = 0;

	if (vkCreateDescriptorPool(RDI->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating descriptor pool failed");
	}
}

void ForwardRenderer::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(RDI->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) // Allocate descriptor sets, each with one uniform buffer descriptor
	{
		ASSERTE(false, "Allocating descriptor sets failed");
	}

	for (size_t i = 0; i < swapChainImages.size(); i++) // Configure each UBO descriptor and each image sampler descriptor
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
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
		renderPassInfo.renderArea.extent = swapChainExtent;

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

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16); // Bind vertex buffers to bindings in shaders

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
	imageAvailableSemaphores.resize(RDI->MAX_FRAMES_IN_FLIGHT); // Set of semaphores for each frame in pool
	renderFinishedSemaphores.resize(RDI->MAX_FRAMES_IN_FLIGHT); // Set of semaphores for each frame in pool
	inFlightFences.resize(RDI->MAX_FRAMES_IN_FLIGHT); // Set of fences for each frame in pool
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE); // Track for each swap chain image if a frame in flight is currently using it to avoid desynchronization when there are more images than fences etc


	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Set fence state to signaled

	for (size_t i = 0; i < RDI->MAX_FRAMES_IN_FLIGHT; i++) // For each frame in pool create sync objects
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



VkSurfaceFormatKHR ForwardRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // Choose the best available mode
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR ForwardRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) // Choose the best available mode (Mailbox is triple buffering)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; //Guaranteed to be available
}

VkExtent2D ForwardRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else // Picks the resolution that best matches the window within the minImageExtent and maxImageExtent bounds
	{
		int width, height;
		SDL_GetWindowSize(RDI->window, &width, &height);
		//glfwGetFramebufferSize(RDI->window, &width, &height); // Query the current size of the framebuffer to make sure that the swapchain images have the (new) right size

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}



VkImageView ForwardRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Describes how to treat images (1D texture, 2D texture, 3D texture or cube map)
	viewInfo.format = format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows you to swizzle the color channels around
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = aspectFlags; // Describes what the image's purpose is and which part of the image should be accessed (Color, depth, stencil, etc)
	viewInfo.subresourceRange.baseMipLevel = 0; // No mipmapping
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(RDI->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		ASSERTE(false, "Creating texture image view failed");
	}

	return imageView;
}



VkFormat ForwardRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(RDI->physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	ASSERTE(false, "Finding supported depth image format failed");
	return candidates[0]; //TODO(HIST0R) what to return? Unreachable code btw
}

VkFormat ForwardRenderer::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool ForwardRenderer::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void ForwardRenderer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Each level will be transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the blit command reading from it is finished

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(RDI->physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) // Check if image format supports linear blitting (since support is not guaranteed on all platforms)
	{
		ASSERTE(false, "Generating mipmaps failed - Texture image format does not support linear blitting");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(RDI->device, commandPool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) // Record VkCmdBlitImage commands generating mmipmaps (Each time generate mipmap one level higher)
	{
		// First, we transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. 
		// This transition will wait for level i - 1 to be filled, either from the previous blit command, or from vkCmdCopyBufferToImage. The current blit command will wait on this transition.

		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 }; // Determine the 3D region that data will be blitted from
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 }; // Determine the 3D region that data will be blitted from
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 }; // Determines the region that data will be blitted to
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }; // Determines the region that data will be blitted to (divided by two since each mip level is half the size of the previous level)
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, // Record blit command
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Transition from
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Transition to
			1, &blit,
			VK_FILTER_LINEAR); // Specify filtering 

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, // This transition waits on the current blit command to finish (All sampling operations will wait on this transition to finish)
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2; // Divide the current mip dimensions by two
		if (mipHeight > 1) mipHeight /= 2; // Divide the current mip dimensions by two
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, // Wait for transition the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(RDI->device, commandPool, RDI->graphicsQueue, commandBuffer);
}

void ForwardRenderer::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D; 
	imageInfo.extent.width = width; 
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format; 
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
	imageInfo.usage = usage;
	imageInfo.samples = numSamples; 
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
	imageInfo.flags = 0;

	if (vkCreateImage(RDI->device, &imageInfo, nullptr, &image) != VK_SUCCESS) 
	{
		ASSERTE(false, "Creating image failed");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(RDI->device, image, &memRequirements);

	VkMemoryAllocateInfo allocationInfo = {};
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.allocationSize = memRequirements.size;
	allocationInfo.memoryTypeIndex = findMemoryType(RDI->memoryProperties, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(RDI->device, &allocationInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		ASSERTE(false, "Allocating image memory failed");
	}

	vkBindImageMemory(RDI->device, image, imageMemory, 0);
}

void ForwardRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(RDI->device, commandPool);

	VkImageMemoryBarrier barrier = {}; // Create image memory barrier to synchronize access to image and transition image layouts  
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout; // Specify layout transition
	barrier.newLayout = newLayout; // Specify layout transition
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For using barrier to transfer queue family ownership
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For using barrier to transfer queue family ownership
	barrier.image = image; // Specify the image that is affected 
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // subresourceRange specify the specific part of the image
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) // Undefined -> transfer destination transition (don't need to wait on anything)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // The earliest possible pipeline stage (is not a real stage, It is a pseudo-stage where transfers happen)
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // Transfer destination -> shader transition (Shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where texture is used)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // Transition should occur in the pipeline transfer stage, since writes don't have to wait on anything)
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Barrier before fragmen shader (since it can read from unfinished image)
	}
	else
	{
		ASSERTE(false, "Transitioning image layout failed - Unsupported layout transition");
	}

	vkCmdPipelineBarrier( // Submit pipeline barrier
		commandBuffer,
		sourceStage, // Specify the pipeline stage in which operations occur that should happen before the barrier
		destinationStage, // Specify the pipeline stage in which operations will wait on the barrier
		0, // (Whether turn the barrier into a per-region condition - That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far)
		0, nullptr, // Specify what to create (memory barriers)
		0, nullptr, // Specify what to create (buffer memory barriers)
		1, &barrier // Specify what to create (image memory barriers)
	);

	endSingleTimeCommands(RDI->device, commandPool, RDI->graphicsQueue, commandBuffer);
}

void ForwardRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) // Specify which part of the buffer is going to be copied to which part of the image and copy them
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(RDI->device, commandPool);

	VkBufferImageCopy region = {}; // Defines the regions of buffers to copy
	region.bufferOffset = 0; // Specify the byte offset in the buffer at which the pixel values start
	region.bufferRowLength = 0; // Specify how the pixels are laid out in memory 
	region.bufferImageHeight = 0; // Specify how the pixels are laid out in memory (padding bytes between rows)
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { // Specify to which part of the image we want to copy the pixels
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region); // Transfer the contents of the buffer to image (The fourth parameter indicates which layout the image is currently using)

	endSingleTimeCommands(RDI->device, commandPool, RDI->graphicsQueue, commandBuffer);
}



//void ForwardRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
//{
//	VkCommandBuffer commandBuffer = beginSingleTimeCommands(RDI->device, commandPool);
//
//	VkBufferCopy copyRegion = {}; // Defines the regions of buffers to copy
//	copyRegion.srcOffset = 0; // Optional
//	copyRegion.dstOffset = 0; // Optional
//	copyRegion.size = size;
//	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion); // Transfer the contents of the buffers
//
//	endSingleTimeCommands(RDI->device, commandPool, RDI->graphicsQueue, commandBuffer);
//}
//
//void ForwardRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
//{
//	VkBufferCreateInfo bufferInfo = {};
//	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	bufferInfo.size = size; // Size of the buffer
//	bufferInfo.usage = usage; // Indicates purpose of the data (possible to specify multiple using bitwise or)
//	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Sharing between different queue families (in this case only graphic queue so no sharing)
//
//	if (vkCreateBuffer(RDI->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
//	{
//		ASSERTE(false, "Creating buffer failed"); // Create a buffer and store a handle in vertexBuffer variable
//	}
//
//	VkMemoryRequirements memRequirements;
//	vkGetBufferMemoryRequirements(RDI->device, buffer, &memRequirements);
//
//	VkMemoryAllocateInfo allocInfo = {}; // Memory allocation parameters
//	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	allocInfo.allocationSize = memRequirements.size;
//	//allocInfo.memoryTypeIndex = findMemoryType(RDI->physicalDevice, memRequirements.memoryTypeBits, properties); // Find memory with flags (eg available to write for CPU)
//	allocInfo.memoryTypeIndex = findMemoryType(RDI->memoryProperties, memRequirements.memoryTypeBits, properties);
//
//
//	if (vkAllocateMemory(RDI->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) // Allocate memory for buffer and store a handle in vertexBufferMemory variable
//	{
//		ASSERTE(false, "Allocating buffer memory failed");
//	}
//
//	vkBindBufferMemory(RDI->device, buffer, bufferMemory, 0); // Associate memory with the buffer (fourth parameter is the offset within the region of memory)
//}



void ForwardRenderer::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
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
	VkResult result = vkAcquireNextImageKHR(RDI->device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); // Start waiting for image from swapchain. When image becomes available sync object is signaled. Last parameter is index of available VkImage in swapchainImages array (using UINT64_MAX for image availability delay disables it)

	if (result == VK_ERROR_OUT_OF_DATE_KHR) // Check if swapchain is still compatible with the surface (and cannot be used for rendering any longer)
	{
		recreateSwapChain();
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

	vkResetFences(RDI->device, 1, &inFlightFences[currentFrame]); // Restore the fence to the unsignaled state

	VkResult queueSubmitResult = vkQueueSubmit(RDI->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
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

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional Allows to specify an array of VkResult to check for every individual swapchain if presentation was successful

	result = vkQueuePresentKHR(RDI->presentQueue, &presentInfo); // Submits the request to present an image to the swap chain

	if (lastViewportRect != sceneView.Rect)
	{
		lastViewportRect = sceneView.Rect;
		Resize(RDI->GetScreenSize());
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized) // Check if swapchain is still compatible with the surface (and cannot be used for rendering any longer)
	{
		windowResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		ASSERTE(false, "Presenting swapchain image failed");
	}


	currentFrame = (currentFrame + 1) % RDI->MAX_FRAMES_IN_FLIGHT; // Advance to the next frame
}


void ForwardRenderer::drawFrame()
{

}