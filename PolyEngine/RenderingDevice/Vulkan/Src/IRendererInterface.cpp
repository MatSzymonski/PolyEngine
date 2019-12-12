#include "PolyRenderingDeviceVKPCH.hpp"
// #include <ForwardRenderer.hpp>

#include "IRendererInterface.hpp"
#include "VKRenderingDevice.hpp"

using namespace Poly;

IRendererInterface::IRendererInterface(VKRenderingDevice * renderingDeviceInterface)
	: RDI(renderingDeviceInterface)
{
	ASSERTE(renderingDeviceInterface != nullptr, "RenderingDeviceInterface passed to Renderer is NULL");
}
