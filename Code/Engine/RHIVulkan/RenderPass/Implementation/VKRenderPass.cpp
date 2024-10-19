#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/RenderPass/VKRenderPass.h>
#include <RHIVulkan/Utilities/VKUtility.h>
#include <RHIVulkan/View/VKView.h>

vk::AttachmentLoadOp Convert(plRHIRenderPassLoadOp op)
{
  switch (op)
  {
    case plRHIRenderPassLoadOp::kLoad:
      return vk::AttachmentLoadOp::eLoad;
    case plRHIRenderPassLoadOp::kClear:
      return vk::AttachmentLoadOp::eClear;
    case plRHIRenderPassLoadOp::kDontCare:
      return vk::AttachmentLoadOp::eDontCare;
  }
  assert(false);
  return vk::AttachmentLoadOp::eLoad;
}

vk::AttachmentStoreOp Convert(plRHIRenderPassStoreOp op)
{
  switch (op)
  {
    case plRHIRenderPassStoreOp::kStore:
      return vk::AttachmentStoreOp::eStore;
    case plRHIRenderPassStoreOp::kDontCare:
      return vk::AttachmentStoreOp::eDontCare;
  }
  assert(false);
  return vk::AttachmentStoreOp::eStore;
}

plVKRenderPass::plVKRenderPass(plVKDevice& device, const plRHIRenderPassDesc& desc)
  : m_desc(desc)
{
  while (!m_desc.colors.empty() && m_desc.colors.back().format == plRHIResourceFormat::UNKNOWN)
  {
    m_desc.colors.pop_back();
  }

  std::vector<vk::AttachmentDescription2> attachment_descriptions;
  auto add_attachment = [&](vk::AttachmentReference2& reference, plRHIResourceFormat::Enum format, vk::ImageLayout layout, plRHIRenderPassLoadOp load_op, plRHIRenderPassStoreOp store_op)
  {
    if (format == plRHIResourceFormat::UNKNOWN)
    {
      reference.attachment = VK_ATTACHMENT_UNUSED;
      return;
    }
    vk::AttachmentDescription2& description = attachment_descriptions.emplace_back();
    description.format = plVKUtils::ToVkFormat(format);
    description.samples = static_cast<vk::SampleCountFlagBits>(m_desc.sampleCount);
    description.loadOp = Convert(load_op);
    description.storeOp = Convert(store_op);
    description.initialLayout = layout;
    description.finalLayout = layout;

    reference.attachment = (plUInt32)(attachment_descriptions.size() - 1);
    reference.layout = layout;
  };

  vk::SubpassDescription2 sub_pass = {};
  sub_pass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

  std::vector<vk::AttachmentReference2> color_attachment_references;
  for (auto& rtv : m_desc.colors)
  {
    add_attachment(color_attachment_references.emplace_back(), rtv.format, vk::ImageLayout::eColorAttachmentOptimal, rtv.loadOp, rtv.storeOp);
  }

  sub_pass.colorAttachmentCount = (plUInt32)color_attachment_references.size();
  sub_pass.pColorAttachments = color_attachment_references.data();

  vk::AttachmentReference2 depth_attachment_reference = {};
  if (m_desc.depthStencil.format != plRHIResourceFormat::UNKNOWN)
  {
    add_attachment(depth_attachment_reference, m_desc.depthStencil.format, vk::ImageLayout::eDepthStencilAttachmentOptimal, m_desc.depthStencil.depthLoadOp, m_desc.depthStencil.depthStoreOp);
    if (depth_attachment_reference.attachment != VK_ATTACHMENT_UNUSED)  
    {
      vk::AttachmentDescription2& description = attachment_descriptions[depth_attachment_reference.attachment];
      description.stencilLoadOp = Convert(m_desc.depthStencil.stencilLoadOp);
      description.stencilStoreOp = Convert(m_desc.depthStencil.depthStoreOp);
    }
    sub_pass.pDepthStencilAttachment = &depth_attachment_reference;
  }

  if (m_desc.shadingRateFormat != plRHIResourceFormat::UNKNOWN)
  {
    vk::AttachmentReference2 shading_rate_image_attachment_reference = {};
    add_attachment(
      shading_rate_image_attachment_reference,
      m_desc.shadingRateFormat,
      vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR,
      plRHIRenderPassLoadOp::kLoad,
      plRHIRenderPassStoreOp::kStore);

    vk::FragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment_info = {};
    fragment_shading_rate_attachment_info.pFragmentShadingRateAttachment = &shading_rate_image_attachment_reference;
    fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.width = device.GetShadingRateImageTileSize();
    fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.height = device.GetShadingRateImageTileSize();
    sub_pass.pNext = &fragment_shading_rate_attachment_info;
  }

  vk::RenderPassCreateInfo2 render_pass_info = {};
  render_pass_info.attachmentCount = (plUInt32)attachment_descriptions.size();
  render_pass_info.pAttachments = attachment_descriptions.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &sub_pass;

  m_render_pass = device.GetDevice().createRenderPass2Unique(render_pass_info);
}

const plRHIRenderPassDesc& plVKRenderPass::GetDesc() const
{
  return m_desc;
}

vk::RenderPass plVKRenderPass::GetRenderPass() const
{
  return m_render_pass.get();
}
