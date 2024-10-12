#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/QueryInterface.h>
#include <RHI/Memory/Memory.h>
#include <RHI/Resource/ResourceStateTracker.h>
#include <RHI/View/View.h>
#include <memory>
#include <string>

struct PL_RHI_DLL plRHIMemoryRequirements
{
  plUInt64 size;
  plUInt64 alignment;
  plUInt32 memoryTypeBits;
};

class PL_RHI_DLL plRHIResource : public plRefCounted
{
public:
  virtual ~plRHIResource() = default;
  virtual void CommitMemory(plRHIMemoryType memory_type) = 0;
  virtual void BindMemory(const plSharedPtr<plRHIMemory>& memory, plUInt64 offset) = 0;
  virtual plRHIResourceType GetResourceType() const = 0;
  virtual plRHIResourceFormat::Enum GetFormat() const = 0;
  virtual plRHIMemoryType GetMemoryType() const = 0;
  virtual plUInt64 GetWidth() const = 0;
  virtual plUInt32 GetHeight() const = 0;
  virtual uint16_t GetLayerCount() const = 0;
  virtual uint16_t GetLevelCount() const = 0;
  virtual plUInt32 GetSampleCount() const = 0;
  virtual plUInt64 GetAccelerationStructureHandle() const = 0;
  virtual void SetName(const plString& name) = 0;
  virtual plUInt8* Map() = 0;
  virtual void Unmap() = 0;
  virtual void UpdateUploadBuffer(plUInt64 bufferOffset, const void* data, plUInt64 numBytes) = 0;
  virtual void UpdateUploadBufferWithTextureData(plUInt64 bufferOffset, plUInt32 bufferRowPitch, plUInt32 bufferDepthPitch,
    const void* srcData, plUInt32 srcRowPitch, plUInt32 srcDepthPitch, plUInt32 numRows, plUInt32 numSlices) = 0;
  virtual bool AllowCommonStatePromotion(plRHIResourceState stateAfter) = 0;
  virtual plRHIResourceState GetInitialState() const = 0;
  virtual plRHIMemoryRequirements GetMemoryRequirements() const = 0;
  virtual bool IsBackBuffer() const = 0;
};
