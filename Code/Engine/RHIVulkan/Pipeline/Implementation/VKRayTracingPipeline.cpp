#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHI/Shader/Shader.h>
#include <RHIVulkan/Adapter/VKAdapter.h>
#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>
#include <RHIVulkan/Pipeline/VKRayTracingPipeline.h>
#include <RHIVulkan/Program/VKProgram.h>

plVKRayTracingPipeline::plVKRayTracingPipeline(plVKDevice& device, const plRHIRayTracingPipelineDesc& desc)
  : plVKPipeline(device, desc.program, desc.layout)
  , m_desc(desc)
{
  std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups(m_desc.groups.size());

  auto get = [&](plUInt64 id) -> plUInt32 {
    auto it = m_shader_ids.Find(id);
    if (it == end(m_shader_ids))
    {
      return VK_SHADER_UNUSED_KHR;
    }
    return it.Value();
  };

  for (size_t i = 0; i < m_desc.groups.size(); ++i)
  {
    decltype(auto) group = groups[i];
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    switch (m_desc.groups[i].type)
    {
      case plRHIRayTracingShaderGroupType::kGeneral:
        group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        group.generalShader = get(m_desc.groups[i].general);
        break;
      case plRHIRayTracingShaderGroupType::kTrianglesHitGroup:
        group.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
        group.closestHitShader = get(m_desc.groups[i].closestHit);
        group.anyHitShader = get(m_desc.groups[i].anyHit);
        break;
      case plRHIRayTracingShaderGroupType::kProceduralHitGroup:
        group.type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
        group.intersectionShader = get(m_desc.groups[i].intersection);
        break;
    }
  }

  vk::RayTracingPipelineCreateInfoKHR ray_pipeline_info{};
  ray_pipeline_info.stageCount = static_cast<plUInt32>(m_shader_stage_create_info.size());
  ray_pipeline_info.pStages = m_shader_stage_create_info.data();
  ray_pipeline_info.groupCount = static_cast<plUInt32>(groups.size());
  ray_pipeline_info.pGroups = groups.data();
  ray_pipeline_info.maxPipelineRayRecursionDepth = 1;
  ray_pipeline_info.layout = m_pipeline_layout;

  m_pipeline = m_device.GetDevice().createRayTracingPipelineKHRUnique({}, {}, ray_pipeline_info).value;
}

plRHIPipelineType plVKRayTracingPipeline::GetPipelineType() const
{
  return plRHIPipelineType::kRayTracing;
}

plDynamicArray<plUInt8> plVKRayTracingPipeline::GetRayTracingShaderGroupHandles(plUInt32 first_group, plUInt32 group_count) const
{
  plDynamicArray<plUInt8> shader_handles_storage;
  shader_handles_storage.SetCountUninitialized(group_count * m_device.GetShaderGroupHandleSize());
  vk::Result result = m_device.GetDevice().getRayTracingShaderGroupHandlesKHR(m_pipeline.get(), first_group, group_count, shader_handles_storage.GetCount(), shader_handles_storage.GetData());
  return shader_handles_storage;
}
