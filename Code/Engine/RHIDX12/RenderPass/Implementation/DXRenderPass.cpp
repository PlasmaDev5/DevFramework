#pragma once
#include <RHIDX12/RenderPass/DXRenderPass.h>

plDXRenderPass::plDXRenderPass(plDXDevice& device, const plRHIRenderPassDesc& desc)
    : m_Desc(desc)
{
}

const plRHIRenderPassDesc& plDXRenderPass::GetDesc() const
{
    return m_Desc;
}
