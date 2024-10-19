#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolTyped.h>
#include <directx/d3dx12.h>

plDXGPUDescriptorPoolTyped::plDXGPUDescriptorPoolTyped(plDXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
  : m_Device(device)
  , m_Type(type)
  , m_Offset(0)
  , m_Size(0)
{
}

plDXGPUDescriptorPoolRange plDXGPUDescriptorPoolTyped::Allocate(plUInt32 count)
{
  auto index = m_EmptyRanges.LowerBound(count);
  if (index != plInvalidIndex)
  {
    plUInt32 offset = m_EmptyRanges[m_EmptyRanges.GetKey(index)];
    plUInt32 size = m_EmptyRanges.GetKey(index);

    // todo: update so plasma can remove all entries with the given key
    while (m_EmptyRanges.RemoveAndCopy(m_EmptyRanges.GetKey(index)))
    {
    }
    return plDXGPUDescriptorPoolRange(*this, m_Device, m_Heap, m_CpuHandle, m_GpuHandle, m_HeapReadable, m_CpuHandleReadable, offset, size, m_Device.GetDevice()->GetDescriptorHandleIncrementSize(m_Type), m_Type);
  }
  if (m_Offset + count > m_Size)
    ResizeHeap(plMath::Max(m_Offset + count, 2 * (m_Size + 1)));
  m_Offset += count;
  return plDXGPUDescriptorPoolRange(*this, m_Device, m_Heap, m_CpuHandle, m_GpuHandle, m_HeapReadable, m_CpuHandleReadable, m_Offset - count, count, m_Device.GetDevice()->GetDescriptorHandleIncrementSize(m_Type), m_Type);
}

void plDXGPUDescriptorPoolTyped::ResizeHeap(plUInt32 reqSize)
{
  if (m_Size >= reqSize)
    return;

  ComPtr<ID3D12DescriptorHeap> heap;
  ComPtr<ID3D12DescriptorHeap> heapReadable;
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = static_cast<plUInt32>(reqSize);
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.Type = m_Type;
  PL_ASSERT_ALWAYS(m_Device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)) == S_OK, "");

  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  PL_ASSERT_ALWAYS(m_Device.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapReadable)) == S_OK, "");

  if (m_Size > 0)
  {
    m_Device.GetDevice()->CopyDescriptorsSimple(m_Size, heapReadable->GetCPUDescriptorHandleForHeapStart(), m_CpuHandleReadable, m_Type);
    m_Device.GetDevice()->CopyDescriptorsSimple(m_Size, heap->GetCPUDescriptorHandleForHeapStart(), m_CpuHandleReadable, m_Type);
  }

  m_Size = heapDesc.NumDescriptors;
  m_Heap = heap;
  m_HeapReadable = heapReadable;
  m_CpuHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();
  m_GpuHandle = m_Heap->GetGPUDescriptorHandleForHeapStart();
  m_CpuHandleReadable = m_HeapReadable->GetCPUDescriptorHandleForHeapStart();
}

void plDXGPUDescriptorPoolTyped::OnRangeDestroy(plUInt32 offset, plUInt32 size)
{
  m_EmptyRanges.Insert(size, offset);
}

void plDXGPUDescriptorPoolTyped::ResetHeap()
{
  m_Offset = 0;
}

ComPtr<ID3D12DescriptorHeap> plDXGPUDescriptorPoolTyped::GetHeap()
{
  return m_Heap;
}
