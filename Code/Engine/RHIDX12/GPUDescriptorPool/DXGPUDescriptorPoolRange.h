#pragma once
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3d12.h>
using namespace Microsoft::WRL;

class plDXDevice;
class plDXGPUDescriptorPoolTyped;

class plDXGPUDescriptorPoolRange : public plRefCounted
{
public:
  using Ptr = plSharedPtr<plDXGPUDescriptorPoolRange>;
  plDXGPUDescriptorPoolRange(
    plDXGPUDescriptorPoolTyped& pool,
    plDXDevice& device,
    ComPtr<ID3D12DescriptorHeap>& heap,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle,
    ComPtr<ID3D12DescriptorHeap>& heapReadable,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandleReadable,
    plUInt32 offset,
    plUInt32 size,
    plUInt32 incrementSize,
    D3D12_DESCRIPTOR_HEAP_TYPE type);
  plDXGPUDescriptorPoolRange(plDXGPUDescriptorPoolRange&& oth);
  ~plDXGPUDescriptorPoolRange();
  void CopyCpuHandle(plUInt32 dstOffset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
  D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(plUInt32 offset = 0) const;

  const ComPtr<ID3D12DescriptorHeap>& GetHeap() const;

  plUInt32 GetSize() const;
  plUInt32 GetOffset() const;

private:
  D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, plUInt32 offset = 0) const;

  std::reference_wrapper<plDXGPUDescriptorPoolTyped> m_Pool;
  std::reference_wrapper<plDXDevice> m_Device;
  std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_Heap;
  std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_CpuHandle;
  std::reference_wrapper<D3D12_GPU_DESCRIPTOR_HANDLE> m_GpuHandle;
  std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_HeapReadable;
  std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_CpuHandleReadable;
  plUInt32 m_Offset;
  plUInt32 m_Size;
  plUInt32 m_IncrementSize;
  D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
  //std::unique_ptr<plDXGPUDescriptorPoolRange, std::function<void(plDXGPUDescriptorPoolRange*)>> m_Callback;
};
