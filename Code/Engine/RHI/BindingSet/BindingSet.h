#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>
#include <vector>

class PL_RHI_DLL plRHIBindingSet : public plRefCounted
{
public:
  virtual ~plRHIBindingSet() = default;
  virtual void WriteBindings(const std::vector<plRHIBindingDesc>& bindings) = 0;
};
