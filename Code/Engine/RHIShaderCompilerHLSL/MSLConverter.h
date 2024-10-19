#pragma once
#include <RHI/Instance/BaseTypes.h>

plString GetMSLShader(const std::vector<plUInt8>& blob, plMap<plString, plUInt32>& mapping);
plString GetMSLShader(const plRHIShaderDesc& shader);
