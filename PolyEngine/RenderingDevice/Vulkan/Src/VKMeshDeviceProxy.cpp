#include <VKMeshDeviceProxy.hpp>

#include <PolyRenderingDeviceVKPCH.hpp>
#include <VKUtils.hpp>
//#include <Resources/Mesh.hpp>

//#include <Common/GLUtils.hpp>

using namespace Poly;
using namespace ::pe::core::math;

VKMeshDeviceProxy::VKMeshDeviceProxy(VKRenderingDevice* RDI)
	: RDI(RDI)
{
	core::utils::gConsole.LogInfo("VKMeshDeviceProxy creating");
}

VKMeshDeviceProxy::~VKMeshDeviceProxy()
{
	//Destroy buffers
}

void VKMeshDeviceProxy::SetContent(const Mesh& mesh)
{
	ASSERTE(mesh.HasVertices() || mesh.HasIndicies(), "Meshes that does not contain vertices or faces are not supported yet!");

	{ // Vertices
		const std::vector<Vector3f> &positions = mesh.GetPositions();
		//const std::vector<::pe::core::math::Vector3f> &vertexColors = mesh.GetVertexColors (); // TODO: Implement retrieving vertex colors in assimp
		const std::vector<Poly::Mesh::TextCoord> &texCoords = mesh.GetTextCoords();

		std::vector<Vertex> vertices;
		for (size_t i = 0; i < mesh.GetVertexCount(); i++)
			vertices.push_back(Vertex(positions[i], Vector3f(0, 0, 0), Vector2f(texCoords[i].U, texCoords[i].V)));

		VkDeviceSize bufferSize = sizeof(Vertex) * mesh.GetVertexCount();
		Buffer stagingBuffer;
		createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		uploadBuffer(stagingBuffer, RDI->device, vertices.data());
		createBuffer(buffers[eBufferType::VERTEX_BUFFER], RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		copyBuffer(stagingBuffer, buffers[eBufferType::VERTEX_BUFFER], bufferSize, RDI->device, RDI->resourcesAllocationCommandPool, RDI->graphicsQueue);
		destroyBuffer(stagingBuffer, RDI->device);
	}

	{ // Indices

		const std::vector<uint32_t> &indices = mesh.GetIndicies();

		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		Buffer stagingBuffer;
		createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		uploadBuffer(stagingBuffer, RDI->device, indices.data());
		createBuffer(buffers[eBufferType::INDEX_BUFFER], RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		copyBuffer(stagingBuffer, buffers[eBufferType::INDEX_BUFFER], bufferSize, RDI->device, RDI->resourcesAllocationCommandPool, RDI->graphicsQueue);
		destroyBuffer(stagingBuffer, RDI->device);
	}

}


