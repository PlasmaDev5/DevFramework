#pragma once
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <directx/d3d12.h>
#include <Foundation/Containers/ArrayMap.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXGPUDescriptorPoolTyped
{
public:
    plDXGPUDescriptorPoolTyped(plDXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    plDXGPUDescriptorPoolRange Allocate(plUInt32 count);
    void ResizeHeap(plUInt32 reqSize);
    void OnRangeDestroy(plUInt32 offset, plUInt32 size);
    void ResetHeap();
    ComPtr<ID3D12DescriptorHeap> GetHeap();

private:
    plDXDevice& m_Device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    plUInt32 m_Offset;
    plUInt32 m_Size;
    ComPtr<ID3D12DescriptorHeap> m_Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
    ComPtr<ID3D12DescriptorHeap> m_HeapReadable;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandleReadable;
    plArrayMap<plUInt32, plUInt32> m_EmptyRanges;
};
