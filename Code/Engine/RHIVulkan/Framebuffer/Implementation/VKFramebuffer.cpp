#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Framebuffer/VKFramebuffer.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/View/VKView.h>
#include <RHIVulkan/Pipeline/VKGraphicsPipeline.h>

plVKFramebuffer::plVKFramebuffer(plVKDevice& device, const plRHIFramebufferDesc& desc)
  : plRHIFramebufferBase(desc)
    , m_extent(desc.width, desc.height)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    std::vector<vk::ImageView> attachment_views;
    framebuffer_info.layers = 1;
    auto add_view = [&](const plSharedPtr<plRHIView>& view)
    {
        if (!view)
            return;
        decltype(auto) vk_view = view.Downcast<plVKView>();
        decltype(auto) resource = vk_view->GetResource();
        if (!resource)
            return;
        attachment_views.emplace_back(vk_view->GetImageView());

        decltype(auto) vk_resource = resource.Downcast<plVKResource>();
        framebuffer_info.layers = plMath::Max(framebuffer_info.layers, vk_resource->image.array_layers);
    };
    for (auto& rtv : desc.colors)
    {
        add_view(rtv);
    }
    add_view(desc.depthStencil);
    add_view(desc.shadingRateImage);

    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.renderPass = desc.renderPass.Downcast<plVKRenderPass>()->GetRenderPass();
    framebuffer_info.attachmentCount = (plUInt32)attachment_views.size();
    framebuffer_info.pAttachments = attachment_views.data();
    m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
}

vk::Framebuffer plVKFramebuffer::GetFramebuffer() const
{
    return m_framebuffer.get();
}

vk::Extent2D plVKFramebuffer::GetExtent() const
{
    return m_extent;
}
