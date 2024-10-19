#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHI/Shader/Shader.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Program/VKProgram.h>

plVKProgram::plVKProgram(plVKDevice& device, const std::vector<plSharedPtr<plRHIShader>>& shaders)
  : plRHIProgramBase(shaders)
{
}
