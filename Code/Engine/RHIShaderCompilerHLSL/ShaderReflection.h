#pragma once
#include <RHIShaderCompilerHLSL/RHIShaderCompilerHLSLDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <memory>
#include <string>
#include <vector>

PL_RHISHADERCOMPILERHLSL_DLL plSharedPtr<plRHIShaderReflection> CreateShaderReflection(plRHIShaderBlobType type, const void* data, size_t size);
