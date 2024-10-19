#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHIVulkan/RHIVulkanDLL.h>

namespace plVKUtils
{
  vk::Format ToVkFormat(plRHIResourceFormat::Enum value);
  plRHIResourceFormat::Enum ToEngineFormat(vk::Format value);
}
