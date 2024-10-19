#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorHandle.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>

plDXCPUDescriptorHandle::plDXCPUDescriptorHandle(
    plDXDevice& device,
    ComPtr<ID3D12DescriptorHeap>& heap,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
    size_t offset,
    size_t size,
    plUInt32 incrementSize,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_Device(device)
    , m_Heap(heap)
    , m_CpuHandle(cpuHandle)
    , m_Offset(offset)
    , m_Size(size)
    , m_IncrementSize(incrementSize)
    , m_Type(type)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE plDXCPUDescriptorHandle::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_CpuHandle,
        static_cast<INT>(m_Offset + offset),
        m_IncrementSize);
}
