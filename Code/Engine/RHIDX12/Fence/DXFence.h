#pragma once
#include <RHI/Fence/Fence.h>
#include <dxgi.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXFence : public plRHIFence
{
public:
    plDXFence(plDXDevice& device, plUInt64 initialValue);
    plUInt64 GetCompletedValue() override;
    void Wait(plUInt64 value) override;
    void Signal(plUInt64 value) override;

    ComPtr<ID3D12Fence> GetFence();

private:
    plDXDevice& m_Device;
    ComPtr<ID3D12Fence> m_pFence;
    HANDLE m_hFenceEvent = nullptr;
};
