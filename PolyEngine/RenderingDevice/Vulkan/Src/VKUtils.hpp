#pragma once

#include <pe/Defines.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Poly {

	uint32_t findMemoryType(VkPhysicalDevice &physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) // Iterate over memory types
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		ASSERTE(false, "Finding suitable memory type failed");
		return 0;
	}










	struct Vertex
	{
		/*::pe::core::math::Vector3f pos;
		::pe::core::math::Vector3f color;
		::pe::core::math::Vector2f texCoord;*/

		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() // Describes at which rate to load data from memory throughout the vertices
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0; // Specifies the index of the binding in the array of bindings
			bindingDescription.stride = sizeof(Vertex); // Specifies the number of bytes from one entry to the next
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Specifies whether to move to the next data entry after each vertex or after each instance

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {}; // Describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description

			//We have two attributes, position and color, so we need two attribute description structs (The binding is loading one Vertex at a time)

			//Vertex position
			attributeDescriptions[0].binding = 0; // From which binding the per-vertex data comes
			attributeDescriptions[0].location = 0; // References the location directive of the input in the vertex shader
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Describes the type of data for the attributE
			attributeDescriptions[0].offset = offsetof(Vertex, pos); // Specifies the number of bytes since the start of the per-vertex data to read from

			//Vertex color
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			//UV coordinates
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject // UniformBufferObject for passing data to shader
	{

		/*	core::math::Matrix model;
			core::math::Matrix view;
			core::math::Matrix proj;*/

		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};



	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};
}


