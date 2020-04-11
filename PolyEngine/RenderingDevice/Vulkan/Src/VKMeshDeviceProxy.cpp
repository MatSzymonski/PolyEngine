#include <VKMeshDeviceProxy.hpp>

#include <PolyRenderingDeviceVKPCH.hpp>
//#include <Resources/Mesh.hpp>

//#include <Common/GLUtils.hpp>

using namespace Poly;

VKMeshDeviceProxy::VKMeshDeviceProxy()
{
	
}

VKMeshDeviceProxy::~VKMeshDeviceProxy()
{
	//Destroy buffers
}

void VKMeshDeviceProxy::SetContent(const Mesh& mesh)
{
	//add device in constructor, command pool, graphics queue

	if (!ID) 
	{
		//Create buffers (vert, ind, etc)
		//If failed
		throw RenderingDeviceProxyCreationFailedException();
	}

	//Push data to buffers

	//buffers[eBufferType::VERTEX_BUFFER] = NULL;
	//buffers[eBufferType::TEXCOORD_BUFFER] = 0;
	//buffers[eBufferType::NORMAL_BUFFER] = 0;
	//buffers[eBufferType::TANGENT_BUFFER] = 0;
	//buffers[eBufferType::INDEX_BUFFER] = 0;
	//buffers[eBufferType::BONE_INDEX_BUFFER] = 0;
	//buffers[eBufferType::BONE_WEIGHT_BUFFER] = 0;

	//ASSERTE(mesh.HasVertices() && mesh.HasIndicies(), "Meshes that does not contain vertices and faces are not supported yet!");

	//
	//if (mesh.HasVertices())
	//{
	//	VkDeviceSize bufferSize = sizeof(::pe::core::math::Vector3f) *  mesh.GetVertexCount();
	//	Buffer stagingBuffer;
	//	createBuffer(stagingBuffer, RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	//	uploadBuffer(stagingBuffer, RDI->device, vertices.data());
	//	createBuffer(buffers[eBufferType::VERTEX_BUFFER], RDI->device, bufferSize, RDI->memoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	//	copyBuffer(stagingBuffer, buffers[eBufferType::VERTEX_BUFFER], bufferSize, RDI->device, commandPool, RDI->graphicsQueue);
	//	destroyBuffer(stagingBuffer, RDI->device);
	//}




	//if (mesh.HasTextCoords())
	//{
	//	EnsureVBOCreated(eBufferType::TEXCOORD_BUFFER);
	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::TEXCOORD_BUFFER]);
	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetTextCoords().size() * sizeof(Mesh::TextCoord), mesh.GetTextCoords().data(), GL_STATIC_DRAW);
	//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	//	glEnableVertexAttribArray(1);
	//	CHECK_GL_ERR();
	//}

	//if (mesh.HasNormals())
	//{
	//	EnsureVBOCreated(eBufferType::NORMAL_BUFFER);
	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::NORMAL_BUFFER]);
	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetNormals().size() * sizeof(::pe::core::math::Vector3f), mesh.GetNormals().data(), GL_STATIC_DRAW);
	//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//	glEnableVertexAttribArray(2);
	//	CHECK_GL_ERR();
	//}

	//if (mesh.HasTangents())
	//{
	//	EnsureVBOCreated(eBufferType::TANGENT_BUFFER);
	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::TANGENT_BUFFER]);
	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetTangents().size() * sizeof(::pe::core::math::Vector3f), mesh.GetTangents().data(), GL_STATIC_DRAW);
	//	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//	glEnableVertexAttribArray(3);
	//	CHECK_GL_ERR();
	//}

	//if (mesh.HasBitangents())
	//{
	//	EnsureVBOCreated(eBufferType::BITANGENT_BUFFER);
	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::BITANGENT_BUFFER]);
	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetBitangents().size() * sizeof(::pe::core::math::Vector3f), mesh.GetBitangents().data(), GL_STATIC_DRAW);
	//	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//	glEnableVertexAttribArray(4);
	//	CHECK_GL_ERR();
	//}

	////TODO(HIST0R) add bones

	//if (mesh.HasIndicies())
	//{
	//	EnsureVBOCreated(eBufferType::INDEX_BUFFER);
	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[eBufferType::INDEX_BUFFER]);
	//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.GetIndicies().size() * sizeof(GLuint), mesh.GetIndicies().data(), GL_STATIC_DRAW);

	//	glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//	glEnableVertexAttribArray(7);
	//	CHECK_GL_ERR();
	//}
}


