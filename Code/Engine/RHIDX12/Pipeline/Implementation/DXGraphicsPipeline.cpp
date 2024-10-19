#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Pipeline/DXGraphicsPipeline.h>
#include <RHIDX12/Pipeline/DXStateBuilder.h>
#include <RHIDX12/Program/DXProgram.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <RHIDX12/View/DXView.h>
#include <directx/d3dx12.h>

CD3DX12_RASTERIZER_DESC GetRasterizerDesc(const plRHIGraphicsPipelineDesc& desc)
{
  CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
  switch (desc.rasterizerDesc.fillMode)
  {
    case plRHIFillMode::kWireframe:
      rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
      break;
    case plRHIFillMode::kSolid:
      rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
      break;
  }
  switch (desc.rasterizerDesc.cullMode)
  {
    case plRHICullMode::kNone:
      rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
      break;
    case plRHICullMode::kFront:
      rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
      break;
    case plRHICullMode::kBack:
      rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
      break;
  }
  rasterizerDesc.DepthBias = desc.rasterizerDesc.depthBias;
  return rasterizerDesc;
}

CD3DX12_BLEND_DESC GetBlendDesc(const plRHIGraphicsPipelineDesc& desc)
{
  CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
  auto convert = [](plRHIBlend type) {
    switch (type)
    {
      case plRHIBlend::kZero:
        return D3D12_BLEND_ZERO;
      case plRHIBlend::kSrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
      case plRHIBlend::kInvSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    }
    return static_cast<D3D12_BLEND>(0);
  };
  auto convertOp = [](plRHIBlendOp type) {
    switch (type)
    {
      case plRHIBlendOp::kAdd:
        return D3D12_BLEND_OP_ADD;
    }
    return static_cast<D3D12_BLEND_OP>(0);
  };
  const plRHIRenderPassDesc& renderPassDesc = desc.renderPass->GetDesc();
  for (size_t i = 0; i < renderPassDesc.colors.size(); ++i)
  {
    if (renderPassDesc.colors[i].format == plRHIResourceFormat::UNKNOWN)
      continue;
    decltype(auto) rtDesc = blendDesc.RenderTarget[i];
    rtDesc.BlendEnable = desc.blendDesc.blendEnable;
    rtDesc.BlendOp = convertOp(desc.blendDesc.blendOp);
    rtDesc.SrcBlend = convert(desc.blendDesc.blendSrc);
    rtDesc.DestBlend = convert(desc.blendDesc.blendDest);
    rtDesc.BlendOpAlpha = convertOp(desc.blendDesc.blendOpAlpha);
    rtDesc.SrcBlendAlpha = convert(desc.blendDesc.blendSrcAlpha);
    rtDesc.DestBlendAlpha = convert(desc.blendDesc.blendDestApha);
    rtDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  }
  return blendDesc;
}

D3D12_COMPARISON_FUNC Convert(plRHIComparisonFunc func)
{
  switch (func)
  {
    case plRHIComparisonFunc::kNever:
      return D3D12_COMPARISON_FUNC_NEVER;
    case plRHIComparisonFunc::kLess:
      return D3D12_COMPARISON_FUNC_LESS;
    case plRHIComparisonFunc::kEqual:
      return D3D12_COMPARISON_FUNC_EQUAL;
    case plRHIComparisonFunc::kLessEqual:
      return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case plRHIComparisonFunc::kGreater:
      return D3D12_COMPARISON_FUNC_GREATER;
    case plRHIComparisonFunc::kNotEqual:
      return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case plRHIComparisonFunc::kGreaterEqual:
      return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case plRHIComparisonFunc::kAlways:
      return D3D12_COMPARISON_FUNC_ALWAYS;
    default:
      assert(false);
      return D3D12_COMPARISON_FUNC_LESS;
  }
}

D3D12_STENCIL_OP Convert(plRHIStencilOp op)
{
  switch (op)
  {
    case plRHIStencilOp::kKeep:
      return D3D12_STENCIL_OP_KEEP;
    case plRHIStencilOp::kZero:
      return D3D12_STENCIL_OP_ZERO;
    case plRHIStencilOp::kReplace:
      return D3D12_STENCIL_OP_REPLACE;
    case plRHIStencilOp::kIncrSat:
      return D3D12_STENCIL_OP_INCR_SAT;
    case plRHIStencilOp::kDecrSat:
      return D3D12_STENCIL_OP_DECR_SAT;
    case plRHIStencilOp::kInvert:
      return D3D12_STENCIL_OP_INVERT;
    case plRHIStencilOp::kIncr:
      return D3D12_STENCIL_OP_INCR;
    case plRHIStencilOp::kDecr:
      return D3D12_STENCIL_OP_DECR;
    default:
      assert(false);
      return D3D12_STENCIL_OP_KEEP;
  }
}

D3D12_DEPTH_STENCILOP_DESC Convert(const plRHIStencilOpDesc& desc)
{
  D3D12_DEPTH_STENCILOP_DESC res = {};
  res.StencilFailOp = Convert(desc.failOp);
  res.StencilPassOp = Convert(desc.passOp);
  res.StencilDepthFailOp = Convert(desc.depthFailOp);
  res.StencilFunc = Convert(desc.func);
  return res;
}

CD3DX12_DEPTH_STENCIL_DESC1 GetDepthStencilDesc(const plRHIDepthStencilDesc& desc, DXGI_FORMAT dsvFormat)
{
  CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc(D3D12_DEFAULT);
  depthStencilDesc.DepthEnable = desc.depthTestEnable;
  depthStencilDesc.DepthWriteMask = desc.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  depthStencilDesc.DepthBoundsTestEnable = desc.depthBoundsTestEnable;
  depthStencilDesc.DepthFunc = Convert(desc.depthFunc);
  depthStencilDesc.StencilEnable = desc.stencilEnable;
  depthStencilDesc.StencilReadMask = desc.stencilReadMask;
  depthStencilDesc.StencilWriteMask = desc.stencilWriteMask;
  depthStencilDesc.FrontFace = Convert(desc.frontFace);
  depthStencilDesc.BackFace = Convert(desc.backFace);

  if (dsvFormat == DXGI_FORMAT_UNKNOWN)
  {
    depthStencilDesc.DepthEnable = false;
  }

  return depthStencilDesc;
}

D3D12_RT_FORMAT_ARRAY GetRTVFormats(const plRHIGraphicsPipelineDesc& desc)
{
  const plRHIRenderPassDesc& renderPassDesc = desc.renderPass->GetDesc();
  D3D12_RT_FORMAT_ARRAY rtFormats = {};
  for (plUInt32 i = 0; i < (plUInt32)renderPassDesc.colors.size(); ++i)
  {
    if (renderPassDesc.colors[i].format == plRHIResourceFormat::UNKNOWN)
      continue;
    rtFormats.NumRenderTargets = i + 1;
    rtFormats.RTFormats[i] = plDXUtils::ToDXGIFormat(renderPassDesc.colors[i].format); //static_cast<DXGI_FORMAT>(gli::dx().translate(renderPassDesc.colors[i].format).DXGIFormat.DDS);
  }
  return rtFormats;
}

DXGI_FORMAT GetDSVFormat(const plRHIGraphicsPipelineDesc& desc)
{
  const plRHIRenderPassDesc& renderPassDesc = desc.renderPass->GetDesc();
  if (renderPassDesc.depthStencil.format == plRHIResourceFormat::UNKNOWN)
    return DXGI_FORMAT_UNKNOWN;
  return plDXUtils::ToDXGIFormat(renderPassDesc.depthStencil.format); //static_cast<DXGI_FORMAT>(gli::dx().translate(renderPassDesc.depthStencil.format).DXGIFormat.DDS);
}

DXGI_SAMPLE_DESC GetSampleDesc(const plRHIGraphicsPipelineDesc& desc)
{
  const plRHIRenderPassDesc& renderPassDesc = desc.renderPass->GetDesc();
  return {renderPassDesc.sampleCount, 0};
}

plDXGraphicsPipeline::plDXGraphicsPipeline(plDXDevice& device, const plRHIGraphicsPipelineDesc& desc)
  : m_Device(device)
  , m_Desc(desc)
{
  plDXStateBuilder graphicsStateBuilder;

  decltype(auto) dxProgram = m_Desc.program.Downcast<plDXProgram>();
  plSharedPtr<plDXBindingSetLayout> dxLayout = m_Desc.layout.Downcast<plDXBindingSetLayout>();
  m_RootSignature = dxLayout->GetRootSignature();
  for (const auto& shader : dxProgram->GetShaders())
  {
    D3D12_SHADER_BYTECODE ShaderBytecode = {};
    decltype(auto) blob = shader->GetBlob();
    ShaderBytecode.pShaderBytecode = blob.GetData();
    ShaderBytecode.BytecodeLength = blob.GetCount();

    switch (shader->GetType())
    {
      case plRHIShaderType::kVertex:
      {
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_VS>(ShaderBytecode);
        ParseInputLayout(shader);
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>(GetInputLayoutDesc());
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
        break;
      }
      case plRHIShaderType::kGeometry:
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_GS>(ShaderBytecode);
        break;
      case plRHIShaderType::kAmplification:
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_AS>(ShaderBytecode);
        break;
      case plRHIShaderType::kMesh:
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_MS>(ShaderBytecode);
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(GetDSVFormat(desc));
        break;
      case plRHIShaderType::kPixel:
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_PS>(ShaderBytecode);
        graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>(GetRTVFormats(desc));
        break;
    }
  }

  graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>(GetDepthStencilDesc(desc.depthStencilDesc, GetDSVFormat(desc)));
  graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>(GetSampleDesc(desc));
  graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(GetRasterizerDesc(desc));
  graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(GetBlendDesc(desc));
  graphicsStateBuilder.AddState<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(m_RootSignature.Get());

  ComPtr<ID3D12Device2> device2;
  m_Device.GetDevice().As(&device2);
  auto psDesc = graphicsStateBuilder.GetDesc();
  PL_ASSERT_ALWAYS(device2->CreatePipelineState(&psDesc, IID_PPV_ARGS(&m_PipelineState)) == S_OK, "");
}

void plDXGraphicsPipeline::ParseInputLayout(const plSharedPtr<plRHIShader>& shader)
{
  for (auto& vertex : m_Desc.input)
  {
    D3D12_INPUT_ELEMENT_DESC layout = {};
    plStringBuilder semanticName = vertex.semanticName.GetData();
    plUInt32 semanticSlot = 0;
    plUInt32 pow = 1;
    while (!semanticName.IsEmpty() && plStringUtils::IsDecimalDigit(semanticName[semanticName.GetCharacterCount() -1]))
    {
      semanticSlot = (semanticName[semanticName.GetCharacterCount() - 1] - '0') * pow + semanticSlot;
      semanticName.Remove(semanticName.GetData() + semanticName.GetCharacterCount() -1, semanticName.GetData() + semanticName.GetCharacterCount());
      pow *= 10;
    }
    m_InputLayoutStride[vertex.slot] = vertex.stride;
    m_InputLayoutDescNames[vertex.slot] = semanticName.GetData();
    layout.SemanticName = m_InputLayoutDescNames[vertex.slot].GetData();
    layout.SemanticIndex = semanticSlot;
    layout.InputSlot = vertex.slot;
    layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    layout.InstanceDataStepRate = 0;
    layout.Format = plDXUtils::ToDXGIFormat(vertex.format); //static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
    m_InputLayoutDesc.push_back(layout);
  }
}

D3D12_INPUT_LAYOUT_DESC plDXGraphicsPipeline::GetInputLayoutDesc()
{
  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
  inputLayoutDesc.NumElements = static_cast<plUInt32>(m_InputLayoutDesc.size());
  inputLayoutDesc.pInputElementDescs = m_InputLayoutDesc.data();
  return inputLayoutDesc;
}

plRHIPipelineType plDXGraphicsPipeline::GetPipelineType() const
{
  return plRHIPipelineType::kGraphics;
}

const plRHIGraphicsPipelineDesc& plDXGraphicsPipeline::GetDesc() const
{
  return m_Desc;
}

const ComPtr<ID3D12PipelineState>& plDXGraphicsPipeline::GetPipeline() const
{
  return m_PipelineState;
}

const ComPtr<ID3D12RootSignature>& plDXGraphicsPipeline::GetRootSignature() const
{
  return m_RootSignature;
}

const plMap<plUInt32, plUInt32>& plDXGraphicsPipeline::GetStrideMap() const
{
  return m_InputLayoutStride;
}
