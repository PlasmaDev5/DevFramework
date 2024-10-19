#pragma once
#include <RHIDX12/CPUDescriptorPool/DXCPUDescriptorHandle.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3dx12.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXCPUDescriptorPoolTyped
{
public:
    plDXCPUDescriptorPoolTyped(plDXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    std::shared_ptr<plDXCPUDescriptorHandle> Allocate(size_t count);
    void ResizeHeap(size_t reqSize);

private:
    plDXDevice& m_Device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    size_t m_Offset;
    size_t m_Size;
    ComPtr<ID3D12DescriptorHeap> m_Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
};
