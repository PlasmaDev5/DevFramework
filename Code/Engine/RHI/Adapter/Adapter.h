#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Device/Device.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>

class plRHIAdapter;

static plSharedPtr<plRHIAdapter> g_GraphicsAdapter = nullptr;

class PL_RHI_DLL plRHIAdapter : public plRefCounted
{
public:
  virtual ~plRHIAdapter() = default;
  virtual const plString& GetName() const = 0;
  virtual plSharedPtr<plRHIDevice> CreateDevice() = 0;
};
