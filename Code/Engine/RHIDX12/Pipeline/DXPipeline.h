#pragma once
#include <RHI/Pipeline/Pipeline.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXPipeline : public plRHIPipeline
{
public:
    virtual ~plDXPipeline() = default;
    virtual const ComPtr<ID3D12RootSignature>& GetRootSignature() const = 0;
    plDynamicArray<plUInt8> GetRayTracingShaderGroupHandles(plUInt32 firstGroup, plUInt32 groupCount) const override;
};
