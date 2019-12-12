//#include "VKMeshDeviceProxy.hpp"
//
//#include "PolyRenderingDeviceVKPCH.hpp"
////#include <Resources/Mesh.hpp>
//
////#include <Common/GLUtils.hpp>
//
//using namespace Poly;
//
//VKMeshDeviceProxy::VKMeshDeviceProxy()
//{
//
//}
//
//VKMeshDeviceProxy::~VKMeshDeviceProxy()
//{
//	
//}
//
//void VKMeshDeviceProxy::SetContent(const Mesh& mesh)
//{
//	//if (!ID) 
//	//{
//	//	//Create buffer
//	//	//If failed
//	//	throw RenderingDeviceProxyCreationFailedException();
//	//}
//
//	//ASSERTE(mesh.HasVertices() && mesh.HasIndicies(), "Meshes that does not contain vertices and faces are not supported yet!");
//
//	//
//	//if (mesh.HasVertices())
//	//{
//	//	EnsureVBOCreated(eBufferType::VERTEX_BUFFER);
//	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::VERTEX_BUFFER]);
//	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetPositions().size() * sizeof(::pe::core::math::Vector3f), mesh.GetPositions().data(), GL_STATIC_DRAW);
//	//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(0);
//	//	CHECK_GL_ERR();
//	//}
//
//	//if (mesh.HasTextCoords())
//	//{
//	//	EnsureVBOCreated(eBufferType::TEXCOORD_BUFFER);
//	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::TEXCOORD_BUFFER]);
//	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetTextCoords().size() * sizeof(Mesh::TextCoord), mesh.GetTextCoords().data(), GL_STATIC_DRAW);
//	//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(1);
//	//	CHECK_GL_ERR();
//	//}
//
//	//if (mesh.HasNormals())
//	//{
//	//	EnsureVBOCreated(eBufferType::NORMAL_BUFFER);
//	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::NORMAL_BUFFER]);
//	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetNormals().size() * sizeof(::pe::core::math::Vector3f), mesh.GetNormals().data(), GL_STATIC_DRAW);
//	//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(2);
//	//	CHECK_GL_ERR();
//	//}
//
//	//if (mesh.HasTangents())
//	//{
//	//	EnsureVBOCreated(eBufferType::TANGENT_BUFFER);
//	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::TANGENT_BUFFER]);
//	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetTangents().size() * sizeof(::pe::core::math::Vector3f), mesh.GetTangents().data(), GL_STATIC_DRAW);
//	//	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(3);
//	//	CHECK_GL_ERR();
//	//}
//
//	//if (mesh.HasBitangents())
//	//{
//	//	EnsureVBOCreated(eBufferType::BITANGENT_BUFFER);
//	//	glBindBuffer(GL_ARRAY_BUFFER, VBO[eBufferType::BITANGENT_BUFFER]);
//	//	glBufferData(GL_ARRAY_BUFFER, mesh.GetBitangents().size() * sizeof(::pe::core::math::Vector3f), mesh.GetBitangents().data(), GL_STATIC_DRAW);
//	//	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(4);
//	//	CHECK_GL_ERR();
//	//}
//
//	////TODO(HIST0R) add bones
//
//	//if (mesh.HasIndicies())
//	//{
//	//	EnsureVBOCreated(eBufferType::INDEX_BUFFER);
//	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[eBufferType::INDEX_BUFFER]);
//	//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.GetIndicies().size() * sizeof(GLuint), mesh.GetIndicies().data(), GL_STATIC_DRAW);
//
//	//	glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	//	glEnableVertexAttribArray(7);
//	//	CHECK_GL_ERR();
//	//}
//}
//
//
