#include <RHIDX12/Memory/DXMemory.h>
#include <RHIDX12/Device/DXDevice.h>
#include <directx/d3dx12.h>

plDXMemory::plDXMemory(plDXDevice& device, plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits)
    : m_MemoryType(memoryType)
{
    D3D12_HEAP_DESC desc = {};
    desc.Properties = CD3DX12_HEAP_PROPERTIES(GetHeapType(memoryType));
    desc.SizeInBytes = size;
    desc.Alignment = memoryTypeBits;
    if (device.IsCreateNotZeroedAvailable())
        desc.Flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    PL_ASSERT_ALWAYS(device.GetDevice()->CreateHeap(&desc, IID_PPV_ARGS(&m_Heap)) == S_OK, "");
}

plRHIMemoryType plDXMemory::GetMemoryType() const
{
    return m_MemoryType;
}

ComPtr<ID3D12Heap> plDXMemory::GetHeap() const
{
    return m_Heap;
}
