#include <RHI/ShaderReflection/ShaderReflection.h>
#ifdef DIRECTX_SUPPORT
#include <RHI/ShaderReflection/DXILReflection.h>
#endif
#if defined(VULKAN_SUPPORT) || defined(METAL_SUPPORT)
#include <RHI/ShaderReflection/SPIRVReflection.h>
#endif
#include <cassert>

std::shared_ptr<plRHIShaderReflection> CreateShaderReflection(plRHIShaderBlobType type, const void* data, size_t size)
{
    switch (type)
    {
    case plRHIShaderBlobType::kDXIL:
#ifdef DIRECTX_SUPPORT
        return std::make_shared<DXILReflection>(data, size);
#else
      // Should never hit but solves build warning
      return nullptr;
#endif
    case plRHIShaderBlobType::kSPIRV:
#if defined(VULKAN_SUPPORT) || defined(METAL_SUPPORT)
        return std::make_shared<SPIRVReflection>(data, size);
#else
      // Should never hit but solves build warning
      return nullptr;
#endif
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
    assert(false);
    return nullptr;
}
