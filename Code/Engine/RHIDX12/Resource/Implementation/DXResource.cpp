#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Memory/DXMemory.h>
#include <RHIDX12/Resource/DXResource.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <directx/d3dx12.h>
#include <optional>

std::optional<D3D12_CLEAR_VALUE> GetClearValue(const D3D12_RESOURCE_DESC& desc)
{
  D3D12_CLEAR_VALUE clearValue = {};
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
  {
    clearValue.Format = desc.Format;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
      clearValue.Color[0] = 0.0f;
      clearValue.Color[1] = 0.0f;
      clearValue.Color[2] = 0.0f;
      clearValue.Color[3] = 1.0f;
      return clearValue;
    }
    else if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
      clearValue.DepthStencil.Depth = 1.0f;
      clearValue.DepthStencil.Stencil = 0;
      clearValue.Format = plDXUtils::DepthStencilFromTypeless(clearValue.Format);
      return clearValue;
    }
  }
  return {};
}

plDXResource::plDXResource(plDXDevice& device)
  : m_Device(device)
{
}

void plDXResource::CommitMemory(plRHIMemoryType memoryType)
{
  m_MemoryType = memoryType;
  auto clearValue = GetClearValue(desc);
  D3D12_CLEAR_VALUE* p_ClearValue = nullptr;
  if (clearValue.has_value())
    p_ClearValue = &clearValue.value();

  // TODO
  if (m_MemoryType == plRHIMemoryType::kUpload)
    SetInitialState(plRHIResourceState::kGenericRead);
  else if (m_MemoryType == plRHIMemoryType::kReadback)
    SetInitialState(plRHIResourceState::kCopyDest);

  D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
  if (m_Device.IsCreateNotZeroedAvailable())
    flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

  auto dx12HeapProperties = CD3DX12_HEAP_PROPERTIES(GetHeapType(m_MemoryType));
  m_Device.GetDevice()->CreateCommittedResource(
    &dx12HeapProperties,
    flags,
    &desc,
    ConvertState(GetInitialState()),
    p_ClearValue,
    IID_PPV_ARGS(&resource));
}

void plDXResource::BindMemory(const plSharedPtr<plRHIMemory>& memory, plUInt64 offset)
{
  m_Memory = memory;
  m_MemoryType = m_Memory->GetMemoryType();
  auto clearValue = GetClearValue(desc);
  D3D12_CLEAR_VALUE* p_ClearValue = nullptr;
  if (clearValue.has_value())
    p_ClearValue = &clearValue.value();

  // TODO
  if (m_MemoryType == plRHIMemoryType::kUpload)
    SetInitialState(plRHIResourceState::kGenericRead);

  plSharedPtr<plDXMemory> dxMemory = m_Memory.Downcast<plDXMemory>();
  m_Device.GetDevice()->CreatePlacedResource(
    dxMemory->GetHeap().Get(),
    offset,
    &desc,
    ConvertState(GetInitialState()),
    p_ClearValue,
    IID_PPV_ARGS(&resource));
}

plUInt64 plDXResource::GetWidth() const
{
  return desc.Width;
}

plUInt32 plDXResource::GetHeight() const
{
  return desc.Height;
}

uint16_t plDXResource::GetLayerCount() const
{
  return desc.DepthOrArraySize;
}

uint16_t plDXResource::GetLevelCount() const
{
  return desc.MipLevels;
}

plUInt32 plDXResource::GetSampleCount() const
{
  return desc.SampleDesc.Count;
}

plUInt64 plDXResource::GetAccelerationStructureHandle() const
{
  return accelerationStructureHandle;
}

void plDXResource::SetName(const plString& name)
{
  if (resource)
  {
    resource->SetName(plStringWChar(name).GetData());
  }
}

plUInt8* plDXResource::Map()
{
  CD3DX12_RANGE range(0, 0);
  plUInt8* dstData = nullptr;
  PL_ASSERT_ALWAYS(resource->Map(0, &range, reinterpret_cast<void**>(&dstData)) == S_OK, "");
  return dstData;
}

void plDXResource::Unmap()
{
  CD3DX12_RANGE range(0, 0);
  resource->Unmap(0, &range);
}

bool plDXResource::AllowCommonStatePromotion(plRHIResourceState stateAfter)
{
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    return true;
  if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
  {
    switch (ConvertState(stateAfter))
    {
      case D3D12_RESOURCE_STATE_DEPTH_WRITE:
        return false;
      default:
        return true;
    }
  }
  else
  {
    switch (ConvertState(stateAfter))
    {
      case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
      case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
      case D3D12_RESOURCE_STATE_COPY_DEST:
      case D3D12_RESOURCE_STATE_COPY_SOURCE:
        return true;
      default:
        return false;
    }
  }
  //return false;
}

plRHIMemoryRequirements plDXResource::GetMemoryRequirements() const
{
  D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_Device.GetDevice()->GetResourceAllocationInfo(0, 1, &desc);
  return {allocationInfo.SizeInBytes, allocationInfo.Alignment, 0};
}
