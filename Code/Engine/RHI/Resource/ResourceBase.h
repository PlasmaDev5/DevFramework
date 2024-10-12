#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Resource/Resource.h>

class PL_RHI_DLL plRHIResourceBase : public plRHIResource
{
public:
  plRHIResourceBase();

  plRHIResourceType GetResourceType() const override final;
  plRHIResourceFormat::Enum GetFormat() const override final;
  plRHIMemoryType GetMemoryType() const override final;

  void UpdateUploadBuffer(plUInt64 bufferOffset, const void* data, plUInt64 numBytes) override final;
  void UpdateUploadBufferWithTextureData(plUInt64 bufferOffset, plUInt32 bufferRowPitch, plUInt32 bufferDepthPitch,
    const void* srcData, plUInt32 srcRowPitch, plUInt32 srcDepthPitch, plUInt32 numRows, plUInt32 numSlices) override final;
  plRHIResourceState GetInitialState() const override final;
  bool IsBackBuffer() const override final;
  void SetInitialState(plRHIResourceState state);
  plRHIResourceStateTracker& GetGlobalResourceStateTracker();
  const plRHIResourceStateTracker& GetGlobalResourceStateTracker() const;

  plRHIResourceFormat::Enum Format = plRHIResourceFormat::UNKNOWN;
  plRHIResourceType ResourceType = plRHIResourceType::kUnknown;
  plSharedPtr<plRHIResource> accelerationStructuresMemory;
  bool m_IsBackBuffer = false;

protected:
  plSharedPtr<plRHIMemory> m_Memory;
  plRHIMemoryType m_MemoryType = plRHIMemoryType::kDefault;

private:
  plRHIResourceStateTracker m_ResourceStateTracker;
  plRHIResourceState m_InitialState = plRHIResourceState::kUnknown;
};
