#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>
#include <RHIVulkan/Program/VKProgram.h>
#include <RHIVulkan/Utilities/VKUtility.h>

vk::CompareOp Convert(plRHIComparisonFunc func)
{
  switch (func)
  {
    case plRHIComparisonFunc::kNever:
      return vk::CompareOp::eNever;
    case plRHIComparisonFunc::kLess:
      return vk::CompareOp::eLess;
    case plRHIComparisonFunc::kEqual:
      return vk::CompareOp::eEqual;
    case plRHIComparisonFunc::kLessEqual:
      return vk::CompareOp::eLessOrEqual;
    case plRHIComparisonFunc::kGreater:
      return vk::CompareOp::eGreater;
    case plRHIComparisonFunc::kNotEqual:
      return vk::CompareOp::eNotEqual;
    case plRHIComparisonFunc::kGreaterEqual:
      return vk::CompareOp::eGreaterOrEqual;
    case plRHIComparisonFunc::kAlways:
      return vk::CompareOp::eAlways;
    default:
      assert(false);
      return vk::CompareOp::eLess;
  }
}

vk::StencilOp Convert(plRHIStencilOp op)
{
  switch (op)
  {
    case plRHIStencilOp::kKeep:
      return vk::StencilOp::eKeep;
    case plRHIStencilOp::kZero:
      return vk::StencilOp::eZero;
    case plRHIStencilOp::kReplace:
      return vk::StencilOp::eReplace;
    case plRHIStencilOp::kIncrSat:
      return vk::StencilOp::eIncrementAndClamp;
    case plRHIStencilOp::kDecrSat:
      return vk::StencilOp::eDecrementAndClamp;
    case plRHIStencilOp::kInvert:
      return vk::StencilOp::eInvert;
    case plRHIStencilOp::kIncr:
      return vk::StencilOp::eIncrementAndWrap;
    case plRHIStencilOp::kDecr:
      return vk::StencilOp::eDecrementAndWrap;
    default:
      assert(false);
      return vk::StencilOp::eKeep;
  }
}

vk::StencilOpState Convert(const plRHIStencilOpDesc& desc, plUInt8 read_mask, plUInt8 write_mask)
{
  vk::StencilOpState res = {};
  res.failOp = Convert(desc.failOp);
  res.passOp = Convert(desc.passOp);
  res.depthFailOp = Convert(desc.depthFailOp);
  res.compareOp = Convert(desc.func);
  res.compareMask = read_mask;
  res.writeMask = write_mask;
  return res;
}

plVKGraphicsPipeline::plVKGraphicsPipeline(plVKDevice& device, const plRHIGraphicsPipelineDesc& desc)
  : plVKPipeline(device, desc.program, desc.layout)
  , m_desc(desc)
{
  if (desc.program->HasShader(plRHIShaderType::kVertex))
  {
    CreateInputLayout(m_binding_desc, m_attribute_desc);
  }

  const plRHIRenderPassDesc& render_pass_desc = m_desc.renderPass->GetDesc();

  vk::PipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.vertexBindingDescriptionCount = (plUInt32)m_binding_desc.size();
  vertex_input_info.pVertexBindingDescriptions = m_binding_desc.data();
  vertex_input_info.vertexAttributeDescriptionCount = (plUInt32)m_attribute_desc.size();
  vertex_input_info.pVertexAttributeDescriptions = m_attribute_desc.data();

  vk::PipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  vk::PipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  vk::PipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.lineWidth = 1.0f;
  rasterizer.frontFace = vk::FrontFace::eClockwise;
  rasterizer.depthBiasEnable = m_desc.rasterizerDesc.depthBias != 0;
  rasterizer.depthBiasConstantFactor = (float)m_desc.rasterizerDesc.depthBias;
  switch (m_desc.rasterizerDesc.fillMode)
  {
    case plRHIFillMode::kWireframe:
      rasterizer.polygonMode = vk::PolygonMode::eLine;
      break;
    case plRHIFillMode::kSolid:
      rasterizer.polygonMode = vk::PolygonMode::eFill;
      break;
  }
  switch (m_desc.rasterizerDesc.cullMode)
  {
    case plRHICullMode::kNone:
      rasterizer.cullMode = vk::CullModeFlagBits::eNone;
      break;
    case plRHICullMode::kFront:
      rasterizer.cullMode = vk::CullModeFlagBits::eFront;
      break;
    case plRHICullMode::kBack:
      rasterizer.cullMode = vk::CullModeFlagBits::eBack;
      break;
  }

  vk::PipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  color_blend_attachment.blendEnable = m_desc.blendDesc.blendEnable;

  if (color_blend_attachment.blendEnable)
  {
    auto convert = [](plRHIBlend type)
    {
      switch (type)
      {
        case plRHIBlend::kZero:
          return vk::BlendFactor::eZero;
        case plRHIBlend::kSrcAlpha:
          return vk::BlendFactor::eSrcAlpha;
        case plRHIBlend::kInvSrcAlpha:
          return vk::BlendFactor::eOneMinusSrcAlpha;
      }
      throw std::runtime_error("unsupported");
    };

    auto convert_op = [](plRHIBlendOp type)
    {
      switch (type)
      {
        case plRHIBlendOp::kAdd:
          return vk::BlendOp::eAdd;
      }
      throw std::runtime_error("unsupported");
    };

    color_blend_attachment.srcColorBlendFactor = convert(m_desc.blendDesc.blendSrc);
    color_blend_attachment.dstColorBlendFactor = convert(m_desc.blendDesc.blendDest);
    color_blend_attachment.colorBlendOp = convert_op(m_desc.blendDesc.blendOp);
    color_blend_attachment.srcAlphaBlendFactor = convert(m_desc.blendDesc.blendSrcAlpha);
    color_blend_attachment.dstAlphaBlendFactor = convert(m_desc.blendDesc.blendDestApha);
    color_blend_attachment.alphaBlendOp = convert_op(m_desc.blendDesc.blendOpAlpha);
  }

  std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments(render_pass_desc.colors.size(), color_blend_attachment);

  vk::PipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = vk::LogicOp::eAnd;
  color_blending.attachmentCount = (plUInt32)color_blend_attachments.size();
  color_blending.pAttachments = color_blend_attachments.data();

  vk::PipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(render_pass_desc.sampleCount);
  multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

  vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
  depth_stencil.depthTestEnable = m_desc.depthStencilDesc.depthTestEnable;
  depth_stencil.depthWriteEnable = m_desc.depthStencilDesc.depthWriteEnable;
  depth_stencil.depthCompareOp = Convert(m_desc.depthStencilDesc.depthFunc);
  depth_stencil.depthBoundsTestEnable = m_desc.depthStencilDesc.depthBoundsTestEnable;
  depth_stencil.stencilTestEnable = m_desc.depthStencilDesc.stencilEnable;
  depth_stencil.back = Convert(m_desc.depthStencilDesc.backFace, m_desc.depthStencilDesc.stencilReadMask, m_desc.depthStencilDesc.stencilWriteMask);
  depth_stencil.front = Convert(m_desc.depthStencilDesc.frontFace, m_desc.depthStencilDesc.stencilReadMask, m_desc.depthStencilDesc.stencilWriteMask);

  std::vector<vk::DynamicState> dynamic_state_enables = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor,
    vk::DynamicState::eFragmentShadingRateKHR,
  };
  vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.pDynamicStates = dynamic_state_enables.data();
  pipelineDynamicStateCreateInfo.dynamicStateCount = (plUInt32)dynamic_state_enables.size();

  vk::GraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.stageCount = (plUInt32)m_shader_stage_create_info.size();
  pipeline_info.pStages = m_shader_stage_create_info.data();
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = m_pipeline_layout;
  pipeline_info.renderPass = GetRenderPass();
  pipeline_info.pDynamicState = &pipelineDynamicStateCreateInfo;

  m_pipeline = m_device.GetDevice().createGraphicsPipelineUnique({}, pipeline_info).value;
}

plRHIPipelineType plVKGraphicsPipeline::GetPipelineType() const
{
  return plRHIPipelineType::kGraphics;
}

vk::RenderPass plVKGraphicsPipeline::GetRenderPass() const
{
  return m_desc.renderPass.Downcast<plVKRenderPass>()->GetRenderPass();
}

void plVKGraphicsPipeline::CreateInputLayout(std::vector<vk::VertexInputBindingDescription>& m_binding_desc, std::vector<vk::VertexInputAttributeDescription>& m_attribute_desc)
{
  for (auto& vertex : m_desc.input)
  {
    decltype(auto) binding = m_binding_desc.emplace_back();
    decltype(auto) attribute = m_attribute_desc.emplace_back();
    attribute.location = m_desc.program->GetShader(plRHIShaderType::kVertex)->GetInputLayoutLocation(vertex.semanticName);
    attribute.binding = binding.binding = vertex.slot;
    binding.inputRate = vk::VertexInputRate::eVertex;
    binding.stride = vertex.stride;
    attribute.format = plVKUtils::ToVkFormat(vertex.format);
  }
}
