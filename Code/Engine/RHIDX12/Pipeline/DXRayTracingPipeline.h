#pragma once
#include <RHIDX12/Pipeline/DXPipeline.h>
#include <RHI/Instance/BaseTypes.h>
#include <directx/d3d12.h>
#include <wrl.h>
#include <set>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXRayTracingPipeline : public plDXPipeline
{
public:
    plDXRayTracingPipeline(plDXDevice& device, const plRHIRayTracingPipelineDesc& desc);
    plRHIPipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;
    plDynamicArray<plUInt8> GetRayTracingShaderGroupHandles(plUInt32 firstGroup, plUInt32 groupCount) const override;

    const ComPtr<ID3D12StateObject>& GetPipeline() const;
    const plRHIRayTracingPipelineDesc& GetDesc() const;

private:
    plString GenerateUniqueName(plString name);

    plDXDevice& m_Device;
    plRHIRayTracingPipelineDesc m_Desc;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12StateObject> m_PipelineState;
    plMap<plUInt64, plString> m_ShaderIds;
    plSet<plString> m_ShaderNames;
    plMap<plUInt64, plString> m_GroupNames;
    ComPtr<ID3D12StateObjectProperties> m_StateOjbectProps;
};
