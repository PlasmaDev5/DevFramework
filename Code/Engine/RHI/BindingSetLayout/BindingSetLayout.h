#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>
#include <vector>

class PL_RHI_DLL plRHIBindingSetLayout : public plRefCounted
{
public:
  virtual ~plRHIBindingSetLayout() = default;
};
