#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHI/Shader/Shader.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Pipeline/VKComputePipeline.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>
#include <RHIVulkan/Program/VKProgram.h>

plVKComputePipeline::plVKComputePipeline(plVKDevice& device, const plRHIComputePipelineDesc& desc)
  : plVKPipeline(device, desc.program, desc.layout)
  , m_desc(desc)
{
  vk::ComputePipelineCreateInfo pipeline_info = {};
  assert(m_shader_stage_create_info.size() == 1);
  pipeline_info.stage = m_shader_stage_create_info.front();
  pipeline_info.layout = m_pipeline_layout;
  m_pipeline = m_device.GetDevice().createComputePipelineUnique({}, pipeline_info).value;
}

plRHIPipelineType plVKComputePipeline::GetPipelineType() const
{
  return plRHIPipelineType::kCompute;
}
