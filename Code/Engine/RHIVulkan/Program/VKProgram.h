#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Program/ProgramBase.h>
#include <RHI/Shader/Shader.h>
#include <vector>

class plVKDevice;

class plVKProgram : public plRHIProgramBase
{
public:
  plVKProgram(plVKDevice& device, const std::vector<plSharedPtr<plRHIShader>>& shaders);
};
