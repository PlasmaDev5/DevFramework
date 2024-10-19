#pragma once
#include <RHI/RenderPass/RenderPass.h>

class plDXDevice;

class plDXRenderPass : public plRHIRenderPass
{
public:
    plDXRenderPass(plDXDevice& device, const plRHIRenderPassDesc& desc);
    const plRHIRenderPassDesc& GetDesc() const override;

private:
    plRHIRenderPassDesc m_Desc;
};
