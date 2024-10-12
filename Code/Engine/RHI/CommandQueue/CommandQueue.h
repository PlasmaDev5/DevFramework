#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/CommandList/CommandList.h>
#include <RHI/Fence/Fence.h>
#include <RHI/Instance/QueryInterface.h>

class PL_RHI_DLL plRHICommandQueue : public plRefCounted
{
public:
  virtual ~plRHICommandQueue() = default;
  virtual void Wait(const plSharedPtr<plRHIFence>& fence, plUInt64 value) = 0;
  virtual void Signal(const plSharedPtr<plRHIFence>& fence, plUInt64 value) = 0;
  virtual void ExecuteCommandLists(const std::vector<plSharedPtr<plRHICommandList>>& commandLists) = 0;
};
