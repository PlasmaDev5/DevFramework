#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/QueryHeap/DXRayTracingQueryHeap.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

plDXRayTracingQueryHeap::plDXRayTracingQueryHeap(plDXDevice& device, plRHIQueryHeapType type, plUInt32 count)
  : m_Device(device)
{
  auto desc = CD3DX12_RESOURCE_DESC::Buffer(count * sizeof(plUInt64));
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  auto dx12HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  m_Device.GetDevice()->CreateCommittedResource(
    &dx12HeapProperties,
    D3D12_HEAP_FLAG_NONE,
    &desc,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    nullptr,
    IID_PPV_ARGS(&m_Resource));
}

plRHIQueryHeapType plDXRayTracingQueryHeap::GetType() const
{
  return plRHIQueryHeapType::kAccelerationStructureCompactedSize;
}

ComPtr<ID3D12Resource> plDXRayTracingQueryHeap::GetResource() const
{
  return m_Resource;
}
