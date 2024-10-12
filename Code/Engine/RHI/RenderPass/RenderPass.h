#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>

class PL_RHI_DLL plRHIRenderPass : public plRefCounted
{
public:
  virtual ~plRHIRenderPass() = default;
  virtual const plRHIRenderPassDesc& GetDesc() const = 0;
};
