#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <RHIVulkan/Pipeline/VKPipeline.h>
#include <RHIVulkan/RenderPass/VKRenderPass.h>

vk::ShaderStageFlagBits ExecutionModel2Bit(plRHIShaderKind kind);

class plVKDevice;

class plVKGraphicsPipeline : public plVKPipeline
{
public:
  plVKGraphicsPipeline(plVKDevice& device, const plRHIGraphicsPipelineDesc& desc);
  plRHIPipelineType GetPipelineType() const override;

  vk::RenderPass GetRenderPass() const;

private:
  void CreateInputLayout(std::vector<vk::VertexInputBindingDescription>& binding_desc, std::vector<vk::VertexInputAttributeDescription>& attribute_desc);

  plRHIGraphicsPipelineDesc m_desc;
  std::vector<vk::VertexInputBindingDescription> m_binding_desc;
  std::vector<vk::VertexInputAttributeDescription> m_attribute_desc;
};
