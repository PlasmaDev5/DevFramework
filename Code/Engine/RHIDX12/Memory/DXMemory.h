#pragma once
#include <RHI/Memory/Memory.h>
#include <RHI/Instance/BaseTypes.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXMemory : public plRHIMemory
{
public:
    plDXMemory(plDXDevice& device, plUInt64 size, plRHIMemoryType memoryType, plUInt32 memoryTypeBits);
    plRHIMemoryType GetMemoryType() const override;
    ComPtr<ID3D12Heap> GetHeap() const;

private:
    plRHIMemoryType m_MemoryType;
    ComPtr<ID3D12Heap> m_Heap;
};
