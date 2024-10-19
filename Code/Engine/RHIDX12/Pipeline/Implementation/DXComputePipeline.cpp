#include <RHIDX12/Pipeline/DXComputePipeline.h>
#include <RHIDX12/Pipeline/DXStateBuilder.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Program/DXProgram.h>
#include <RHI/Shader/Shader.h>
#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/View/DXView.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <directx/d3dx12.h>

plDXComputePipeline::plDXComputePipeline(plDXDevice& device, const plRHIComputePipelineDesc& desc)
    : m_Device(device)
    , m_Desc(desc)
{
    plDXStateBuilder computeStateBuilder;

    decltype(auto) dxProgram = m_Desc.program.Downcast<plDXProgram>();
    plSharedPtr<plDXBindingSetLayout> dxLayout = m_Desc.layout.Downcast<plDXBindingSetLayout>();
    m_RootSignature = dxLayout->GetRootSignature();
    for (const auto& shader : dxProgram->GetShaders())
    {
        D3D12_SHADER_BYTECODE shaderBytecode = {};
        decltype(auto) blob = shader->GetBlob();
        shaderBytecode.pShaderBytecode = blob.GetData();
        shaderBytecode.BytecodeLength = blob.GetCount();
        switch (shader->GetType())
        {
        case plRHIShaderType::kCompute:
        {
            computeStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_CS>(shaderBytecode);
            break;
        }
        }
    }
    computeStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(m_RootSignature.Get());

    ComPtr<ID3D12Device2> device2;
    m_Device.GetDevice().As(&device2);
    auto psDesc = computeStateBuilder.GetDesc();
    PL_ASSERT_ALWAYS(device2->CreatePipelineState(&psDesc, IID_PPV_ARGS(&m_PipelineState)) == S_OK, "");
}

plRHIPipelineType plDXComputePipeline::GetPipelineType() const
{
    return plRHIPipelineType::kCompute;
}

const plRHIComputePipelineDesc& plDXComputePipeline::GetDesc() const
{
    return m_Desc;
}

const ComPtr<ID3D12PipelineState>& plDXComputePipeline::GetPipeline() const
{
    return m_PipelineState;
}

const ComPtr<ID3D12RootSignature>& plDXComputePipeline::GetRootSignature() const
{
    return m_RootSignature;
}
