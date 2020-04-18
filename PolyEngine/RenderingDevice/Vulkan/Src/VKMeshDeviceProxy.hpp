#pragma once

#include <pe/Defines.hpp>
//#include <Common/GLUtils.hpp>
#include <Resources.hpp>
#include "VKRenderingDevice.hpp"

namespace Poly
{
	class VKMeshDeviceProxy : public IMeshDeviceProxy
	{
	private:
		enum class eBufferType {
			VERTEX_BUFFER,
			TEXCOORD_BUFFER,
			NORMAL_BUFFER,
			TANGENT_BUFFER,
			BITANGENT_BUFFER,
			INDEX_BUFFER,
			BONE_INDEX_BUFFER,
			BONE_WEIGHT_BUFFER,
			_COUNT
		};

	public:
		VKMeshDeviceProxy(VKRenderingDevice* RDI);
		virtual ~VKMeshDeviceProxy();
		void SetContent(const Mesh& mesh);
		unsigned int GetResourceID() const { return ID; };

	private:		
		uint32_t ID = 0;
		::pe::core::utils::EnumArray<Buffer, eBufferType> buffers;

		VKRenderingDevice* RDI;

		
	};
}