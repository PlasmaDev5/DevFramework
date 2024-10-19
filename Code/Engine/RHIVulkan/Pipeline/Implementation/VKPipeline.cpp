#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Pipeline/VKPipeline.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>

vk::ShaderStageFlagBits ExecutionModel2Bit(plRHIShaderKind kind)
{
    switch (kind)
    {
      case plRHIShaderKind::kVertex:
        return vk::ShaderStageFlagBits::eVertex;
      case plRHIShaderKind::kPixel:
        return vk::ShaderStageFlagBits::eFragment;
      case plRHIShaderKind::kCompute:
        return vk::ShaderStageFlagBits::eCompute;
      case plRHIShaderKind::kGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
      case plRHIShaderKind::kAmplification:
        return vk::ShaderStageFlagBits::eTaskNV;
      case plRHIShaderKind::kMesh:
        return vk::ShaderStageFlagBits::eMeshNV;
      case plRHIShaderKind::kRayGeneration:
        return vk::ShaderStageFlagBits::eRaygenKHR;
      case plRHIShaderKind::kIntersection:
        return vk::ShaderStageFlagBits::eIntersectionKHR;
      case plRHIShaderKind::kAnyHit:
        return vk::ShaderStageFlagBits::eAnyHitKHR;
      case plRHIShaderKind::kClosestHit:
        return vk::ShaderStageFlagBits::eClosestHitKHR;
      case plRHIShaderKind::kMiss:
        return vk::ShaderStageFlagBits::eMissKHR;
      case plRHIShaderKind::kCallable:
        return vk::ShaderStageFlagBits::eCallableKHR;
    }
    assert(false);
    return {};
}

plVKPipeline::plVKPipeline(plVKDevice& device, const plSharedPtr<plRHIProgram>& program, const plSharedPtr<plRHIBindingSetLayout>& layout)
    : m_device(device)
{
    decltype(auto) vk_layout = layout.Downcast<plVKBindingSetLayout>();
    m_pipeline_layout = vk_layout->GetPipelineLayout();

    decltype(auto) shaders = program->GetShaders();
    for (const auto& shader : shaders)
    {
        decltype(auto) blob = shader->GetBlob();
        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = blob.GetCount();
        shader_module_info.pCode = (plUInt32*)blob.GetData();
        m_shader_modules.emplace_back(m_device.GetDevice().createShaderModuleUnique(shader_module_info));

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) entry_points = reflection->GetEntryPoints();
        for (const auto& entry_point : entry_points)
        {
          m_shader_ids[shader->GetId(entry_point.name)] = (plUInt32)m_shader_stage_create_info.size();
            decltype(auto) shader_stage_create_info = m_shader_stage_create_info.emplace_back();
            shader_stage_create_info.stage = ExecutionModel2Bit(entry_point.kind);
            shader_stage_create_info.module = m_shader_modules.back().get();
            decltype(auto) name = entry_point_names.emplace_back(entry_point.name);
            shader_stage_create_info.pName = name.GetData();
        }
    }
}

vk::Pipeline plVKPipeline::GetPipeline() const
{
    return m_pipeline.get();
}

vk::PipelineLayout plVKPipeline::GetPipelineLayout() const
{
    return m_pipeline_layout;
}

plDynamicArray<plUInt8> plVKPipeline::GetRayTracingShaderGroupHandles(plUInt32 first_group, plUInt32 group_count) const
{
    return {};
}
