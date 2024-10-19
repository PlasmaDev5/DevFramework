#pragma once
#include <RHI/QueryHeap/QueryHeap.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXRayTracingQueryHeap : public plRHIQueryHeap
{
public:
    plDXRayTracingQueryHeap(plDXDevice& device, plRHIQueryHeapType type, plUInt32 count);

    plRHIQueryHeapType GetType() const override;

    ComPtr<ID3D12Resource> GetResource() const;

private:
    plDXDevice& m_Device;
    ComPtr<ID3D12Resource> m_Resource;
};

