#pragma once
#include <RHIDX12/Pipeline/DXPipeline.h>
#include <RHI/Instance/BaseTypes.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;
class plRHIShader;

class plDXGraphicsPipeline : public plDXPipeline
{
public:
    plDXGraphicsPipeline(plDXDevice& device, const plRHIGraphicsPipelineDesc& desc);
    plRHIPipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;

    const plRHIGraphicsPipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;
    const plMap<plUInt32, plUInt32>& GetStrideMap() const;

private:
    void ParseInputLayout(const plSharedPtr<plRHIShader>& shader);
    D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc();

    plDXDevice& m_Device;
    plRHIGraphicsPipelineDesc m_Desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayoutDesc;
    plMap<plUInt32, plString> m_InputLayoutDescNames;
    plMap<plUInt32, plUInt32> m_InputLayoutStride;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
