#include <RHI/Framebuffer/FramebufferBase.h>

#include <RHI/RenderPass/RenderPass.h>

plRHIFramebufferBase::plRHIFramebufferBase(const plRHIFramebufferDesc& desc)
    : m_Desc(desc)
{
}

const plRHIFramebufferDesc& plRHIFramebufferBase::GetDesc() const
{
    return m_Desc;
}
