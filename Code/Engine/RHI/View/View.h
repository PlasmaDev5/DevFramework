#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <memory>

class plRHIResource;

class PL_RHI_DLL plRHIView : public plRefCounted
{
public:
  virtual ~plRHIView() = default;
  virtual plSharedPtr<plRHIResource> GetResource() = 0;
  virtual plUInt32 GetDescriptorId() const = 0;
  virtual plUInt32 GetBaseMipLevel() const = 0;
  virtual plUInt32 GetLevelCount() const = 0;
  virtual plUInt32 GetBaseArrayLayer() const = 0;
  virtual plUInt32 GetLayerCount() const = 0;
};
