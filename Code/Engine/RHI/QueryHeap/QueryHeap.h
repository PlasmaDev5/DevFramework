#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>

class PL_RHI_DLL plRHIQueryHeap : public plRefCounted
{
public:
  virtual ~plRHIQueryHeap() = default;
  virtual plRHIQueryHeapType GetType() const = 0;
};
