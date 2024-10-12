#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/QueryInterface.h>
#include <memory>

class PL_RHI_DLL plRHIFramebuffer : public plRefCounted
{
public:
  virtual ~plRHIFramebuffer() = default;
};
