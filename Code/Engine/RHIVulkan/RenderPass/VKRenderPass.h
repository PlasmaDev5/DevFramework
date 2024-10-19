#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/RenderPass/RenderPass.h>

class plVKDevice;

class plVKRenderPass : public plRHIRenderPass
{
public:
  plVKRenderPass(plVKDevice& device, const plRHIRenderPassDesc& desc);
  const plRHIRenderPassDesc& GetDesc() const override;

    vk::RenderPass GetRenderPass() const;

private:
    plRHIRenderPassDesc m_desc;
    vk::UniqueRenderPass m_render_pass;
};
