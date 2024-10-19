#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Framebuffer/FramebufferBase.h>

class plVKDevice;
class plVKGraphicsPipeline;

class plVKFramebuffer : public plRHIFramebufferBase
{
public:
  plVKFramebuffer(plVKDevice& device, const plRHIFramebufferDesc& desc);

    vk::Framebuffer GetFramebuffer() const;
    vk::Extent2D GetExtent() const;

private:
    vk::UniqueFramebuffer m_framebuffer;
    vk::Extent2D m_extent;
};
