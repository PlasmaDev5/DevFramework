#pragma once
#include <RHIDX12/Pipeline/DXPipeline.h>
#include <RHI/Instance/BaseTypes.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXComputePipeline : public plDXPipeline
{
public:
    plDXComputePipeline(plDXDevice& device, const plRHIComputePipelineDesc& desc);
    plRHIPipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;

    const plRHIComputePipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;

private:
    plDXDevice& m_Device;
    plRHIComputePipelineDesc m_Desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayoutDesc;
    plMap<size_t, std::string> m_InputLayoutDescNames;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
