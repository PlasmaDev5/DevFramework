#pragma once
#include <RHI/Resource/ResourceBase.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXResource : public plRHIResourceBase
{
public:
    plDXResource(plDXDevice& device);

    void CommitMemory(plRHIMemoryType memoryType) override;
    void BindMemory(const plSharedPtr<plRHIMemory>& memory, plUInt64 offset) override;
    plUInt64 GetWidth() const override;
    plUInt32 GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    plUInt32 GetSampleCount() const override;
    plUInt64 GetAccelerationStructureHandle() const override;
    void SetName(const plString& name) override;
    plUInt8* Map() override;
    void Unmap() override;
    bool AllowCommonStatePromotion(plRHIResourceState stateAfter) override;
    plRHIMemoryRequirements GetMemoryRequirements() const override;

    ComPtr<ID3D12Resource> resource;
    D3D12_RESOURCE_DESC desc = {};
    D3D12_SAMPLER_DESC samplerDesc = {};
    D3D12_GPU_VIRTUAL_ADDRESS accelerationStructureHandle = {};

private:
    plDXDevice& m_Device;
};
