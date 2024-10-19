#pragma once
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3d12.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXCPUDescriptorHandle
{
public:
    plDXCPUDescriptorHandle(
        plDXDevice& device,
        ComPtr<ID3D12DescriptorHeap>& heap,
        D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
        size_t offset,
        size_t size,
        plUInt32 incrementSize,
        D3D12_DESCRIPTOR_HEAP_TYPE type);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;

private:
    std::reference_wrapper<plDXDevice> m_Device;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_Heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_CpuHandle;
    size_t m_Offset;
    size_t m_Size;
    plUInt32 m_IncrementSize;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
};
