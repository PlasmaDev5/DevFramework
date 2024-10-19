#pragma once

#include <RHIShaderCompilerHLSL/RHIShaderCompilerHLSLDLL.h>
#include <RHI/Instance/BaseTypes.h>

PL_RHISHADERCOMPILERHLSL_DLL plDynamicArray<plUInt8> Compile(plRHIShaderDesc shader, plRHIShaderBlobType blobType);
