#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolTyped.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>

plDXGPUDescriptorPoolRange::plDXGPUDescriptorPoolRange(
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
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_Pool(pool)
    , m_Device(device)
    , m_Heap(heap)
    , m_CpuHandle(cpuHandle)
    , m_GpuHandle(gpuHandle)
    , m_HeapReadable(heapReadable)
    , m_CpuHandleReadable(cpuHandleReadable)
    , m_Offset(offset)
    , m_Size(size)
    , m_IncrementSize(incrementSize)
    , m_Type(type)
    //, m_Callback(this, [m_Offset = m_Offset, m_Size = m_Size, m_Pool = m_Pool](auto) { m_Pool.get().OnRangeDestroy(m_Offset, m_Size); })
{
}

plDXGPUDescriptorPoolRange::plDXGPUDescriptorPoolRange(plDXGPUDescriptorPoolRange&& oth) = default;

plDXGPUDescriptorPoolRange::~plDXGPUDescriptorPoolRange() {
  m_Pool.get().OnRangeDestroy(m_Offset, m_Size);
}

void plDXGPUDescriptorPoolRange::CopyCpuHandle(plUInt32 dstOffset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE self = GetCpuHandle(m_CpuHandle, dstOffset);
    m_Device.get().GetDevice()->CopyDescriptors(
        1, &self, nullptr,
        1, &handle, nullptr,
        m_Type);
    self = GetCpuHandle(m_CpuHandleReadable, dstOffset);
    m_Device.get().GetDevice()->CopyDescriptors(
        1, &self, nullptr,
        1, &handle, nullptr,
        m_Type);
}

D3D12_CPU_DESCRIPTOR_HANDLE plDXGPUDescriptorPoolRange::GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, plUInt32 offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        handle,
        static_cast<INT>(m_Offset + offset),
        m_IncrementSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE plDXGPUDescriptorPoolRange::GetGpuHandle(plUInt32 offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_GpuHandle,
        static_cast<INT>(m_Offset + offset),
        m_IncrementSize);
}

const ComPtr<ID3D12DescriptorHeap>& plDXGPUDescriptorPoolRange::GetHeap() const
{
    return m_Heap;
}

plUInt32 plDXGPUDescriptorPoolRange::GetSize() const
{
    return m_Size;
}

plUInt32 plDXGPUDescriptorPoolRange::GetOffset() const
{
    return m_Offset;
}
