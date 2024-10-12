#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Fence/Fence.h>
#include <RHI/Instance/QueryInterface.h>
#include <RHI/Resource/Resource.h>

using plRHIWindow = void*;

class PL_RHI_DLL plRHISwapchain : public plRefCounted
{
public:
  virtual ~plRHISwapchain() = default;
  virtual plRHIResourceFormat::Enum GetFormat() const = 0;
  virtual plSharedPtr<plRHIResource> GetBackBuffer(plUInt32 buffer) = 0;
  virtual plUInt32 NextImage(const plSharedPtr<plRHIFence>& fence, plUInt64 signalValue) = 0;
  virtual void Present(const plSharedPtr<plRHIFence>& fence, plUInt64 waitValue) = 0;
};
