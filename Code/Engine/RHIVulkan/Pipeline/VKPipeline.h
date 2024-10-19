#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Pipeline/Pipeline.h>
#include <RHI/Program/Program.h>
#include <deque>

class plVKDevice;

class plVKPipeline : public plRHIPipeline
{
public:
  plVKPipeline(plVKDevice& device, const plSharedPtr<plRHIProgram>& program, const plSharedPtr<plRHIBindingSetLayout>& layout);
  vk::PipelineLayout GetPipelineLayout() const;
  vk::Pipeline GetPipeline() const;
  plDynamicArray<plUInt8> GetRayTracingShaderGroupHandles(plUInt32 first_group, plUInt32 group_count) const override;

protected:
  plVKDevice& m_device;
  std::deque<plString> entry_point_names;
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_info;
  std::vector<vk::UniqueShaderModule> m_shader_modules;
  vk::UniquePipeline m_pipeline;
  vk::PipelineLayout m_pipeline_layout;
  plMap<plUInt64, plUInt32> m_shader_ids;
};
