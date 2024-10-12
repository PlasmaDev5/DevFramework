#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Framebuffer/Framebuffer.h>
#include <RHI/View/View.h>
#include <vector>

class PL_RHI_DLL plRHIFramebufferBase
  : public plRHIFramebuffer
{
public:
  plRHIFramebufferBase(const plRHIFramebufferDesc& desc);
  const plRHIFramebufferDesc& GetDesc() const;

private:
  plRHIFramebufferDesc m_Desc;
};
