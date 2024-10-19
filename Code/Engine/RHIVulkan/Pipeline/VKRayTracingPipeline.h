#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <RHIVulkan/Pipeline/VKPipeline.h>
#include <RHIVulkan/RenderPass/VKRenderPass.h>

class plVKDevice;

class plVKRayTracingPipeline : public plVKPipeline
{
public:
  plVKRayTracingPipeline(plVKDevice& device, const plRHIRayTracingPipelineDesc& desc);
  plRHIPipelineType GetPipelineType() const override;
  plDynamicArray<plUInt8> GetRayTracingShaderGroupHandles(plUInt32 first_group, plUInt32 group_count) const override;

private:
  plRHIRayTracingPipelineDesc m_desc;
};
