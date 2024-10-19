#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <RHIVulkan/Pipeline/VKPipeline.h>
#include <RHIVulkan/RenderPass/VKRenderPass.h>

class plVKDevice;

class plVKComputePipeline : public plVKPipeline
{
public:
  plVKComputePipeline(plVKDevice& device, const plRHIComputePipelineDesc& desc);
  plRHIPipelineType GetPipelineType() const override;

private:
  plRHIComputePipelineDesc m_desc;
};
