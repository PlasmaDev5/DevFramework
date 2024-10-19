#include <RHIShaderCompilerHLSL/ShaderReflection.h>
#ifdef DIRECTX_SUPPORT
#include <RHIShaderCompilerHLSL/DXILReflection.h>
#endif
#ifdef VULKAN_SUPPORT
#include <RHIShaderCompilerHLSL/SPIRVReflection.h>
#endif
#include <cassert>

plSharedPtr<plRHIShaderReflection> CreateShaderReflection(plRHIShaderBlobType type, const void* data, size_t size)
{

#ifdef DIRECTX_SUPPORT
    if(type == plRHIShaderBlobType::kDXIL)
        return PL_DEFAULT_NEW(DXILReflection, data, size);
#endif
#ifdef VULKAN_SUPPORT
    if(type == plRHIShaderBlobType::kSPIRV)
        return PL_DEFAULT_NEW(SPIRVReflection, data, size);
#endif

    return nullptr;
}
