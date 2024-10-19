#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <RHIDX12/Program/DXProgram.h>
#include <deque>
#include <stdexcept>
#include <directx/d3dx12.h>

D3D12_SHADER_VISIBILITY GetVisibility(plRHIShaderType shaderType)
{
  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
  switch (shaderType)
  {
    case plRHIShaderType::kVertex:
      visibility = D3D12_SHADER_VISIBILITY_VERTEX;
      break;
    case plRHIShaderType::kPixel:
      visibility = D3D12_SHADER_VISIBILITY_PIXEL;
      break;
    case plRHIShaderType::kCompute:
      visibility = D3D12_SHADER_VISIBILITY_ALL;
      break;
    case plRHIShaderType::kGeometry:
      visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
      break;
    case plRHIShaderType::kAmplification:
      visibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
      break;
    case plRHIShaderType::kMesh:
      visibility = D3D12_SHADER_VISIBILITY_MESH;
      break;
  }
  return visibility;
}

D3D12_DESCRIPTOR_RANGE_TYPE GetRangeType(plRHIViewType viewType)
{
  //D3D12_DESCRIPTOR_RANGE_TYPE range_type;

  switch (viewType)
  {
    case plRHIViewType::kTexture:
    case plRHIViewType::kBuffer:
    case plRHIViewType::kStructuredBuffer:
    case plRHIViewType::kAccelerationStructure:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case plRHIViewType::kRWTexture:
    case plRHIViewType::kRWBuffer:
    case plRHIViewType::kRWStructuredBuffer:
      return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case plRHIViewType::kConstantBuffer:
      return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case plRHIViewType::kSampler:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:
      throw std::runtime_error("wrong view type");
  }
}

D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType(plRHIViewType viewType)
{
  //D3D12_DESCRIPTOR_RANGE_TYPE range_type;
  switch (GetRangeType(viewType))
  {
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    default:
      throw std::runtime_error("wrong view type");
  }
}

plDXBindingSetLayout::plDXBindingSetLayout(plDXDevice& device, const std::vector<plRHIBindKey>& descs)
  : m_Device(device)
{
  std::vector<D3D12_ROOT_PARAMETER> rootParameters;
  using RootKey = std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, plRHIShaderType>;
  plMap<RootKey, std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorTableRanges;
  plMap<RootKey, plUInt32> descriptorTableOffset;
  std::deque<D3D12_DESCRIPTOR_RANGE> bindlessRanges;

  auto addRootTable = [&](plRHIShaderType shaderType, plUInt32 rangeCount, const D3D12_DESCRIPTOR_RANGE* ranges) {
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable = {};
    descriptorTable.NumDescriptorRanges = rangeCount;
    descriptorTable.pDescriptorRanges = ranges;

    plUInt32 rootParamIndex = (plUInt32)rootParameters.size();
    decltype(auto) rootParameter = rootParameters.emplace_back();
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter.DescriptorTable = descriptorTable;
    rootParameter.ShaderVisibility = GetVisibility(shaderType);
    return rootParamIndex;
  };

  auto addBindlessRange = [&](plRHIShaderType shaderType, plRHIViewType viewType, plUInt32 baseSlot, plUInt32 space) {
    auto& descriptorTableRange = bindlessRanges.emplace_back();
    descriptorTableRange.RangeType = GetRangeType(viewType);
    descriptorTableRange.NumDescriptors = UINT_MAX;
    descriptorTableRange.BaseShaderRegister = baseSlot;
    descriptorTableRange.RegisterSpace = space;
    plUInt32 rootParamIndex = addRootTable(shaderType, 1, &descriptorTableRange);
    m_DescriptorTables[rootParamIndex].heapType = GetHeapType(viewType);
    m_DescriptorTables[rootParamIndex].heapOffset = 0;
    m_DescriptorTables[rootParamIndex].bindless = true;
    switch (shaderType)
    {
      case plRHIShaderType::kCompute:
      case plRHIShaderType::kLibrary:
        m_DescriptorTables[rootParamIndex].isCompute = true;
        break;
    }
  };

  for (const auto& bindKey : descs)
  {
    if (bindKey.count == plMath::MaxValue<plUInt32>())
    {
      addBindlessRange(bindKey.shaderType, bindKey.viewType, bindKey.slot, bindKey.space);
      continue;
    }

    D3D12_DESCRIPTOR_HEAP_TYPE heapType = GetHeapType(bindKey.viewType);
    decltype(auto) layout = m_Layout[bindKey];
    layout.heapType = heapType;
    layout.heapOffset = m_HeapDescs[heapType];

    RootKey key = {heapType, bindKey.shaderType};
    if (!descriptorTableOffset.Contains(key))
    {
      descriptorTableOffset[key] = m_HeapDescs[heapType];
    }

    decltype(auto) range = descriptorTableRanges[key].emplace_back();
    range.RangeType = GetRangeType(bindKey.viewType);
    range.NumDescriptors = bindKey.count;
    range.BaseShaderRegister = bindKey.slot;
    range.RegisterSpace = bindKey.space;
    range.OffsetInDescriptorsFromTableStart = layout.heapOffset - descriptorTableOffset[key];

    m_HeapDescs[heapType] += bindKey.count;
  }

  for (const auto& ranges : descriptorTableRanges)
  {
    plUInt32 rootParamIndex = addRootTable(ranges.Key().second, (plUInt32)ranges.Value().size(), ranges.Value().data());
    m_DescriptorTables[rootParamIndex].heapType = ranges.Key().first;
    m_DescriptorTables[rootParamIndex].heapOffset = descriptorTableOffset[ranges.Key()];
    switch (ranges.Key().second)
    {
      case plRHIShaderType::kCompute:
      case plRHIShaderType::kLibrary:
        m_DescriptorTables[rootParamIndex].isCompute = true;
        break;
    }
  }

  D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
  rootSignatureDesc.Init(static_cast<plUInt32>(rootParameters.size()),
    rootParameters.data(),
    0,
    nullptr,
    rootSignatureFlags);

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> errorBlob;
  PL_ASSERT_ALWAYS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBlob) == S_OK,
    "{}", static_cast<char*>(errorBlob->GetBufferPointer()));
  PL_ASSERT_ALWAYS(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)) == S_OK, "");
}

const plMap<D3D12_DESCRIPTOR_HEAP_TYPE, plUInt32>& plDXBindingSetLayout::GetHeapDescs() const
{
  return m_HeapDescs;
}

const plMap<plRHIBindKey, plBindingLayout>& plDXBindingSetLayout::GetLayout() const
{
  return m_Layout;
}

const plMap<plUInt32, plDescriptorTableDesc>& plDXBindingSetLayout::GetDescriptorTables() const
{
  return m_DescriptorTables;
}

const ComPtr<ID3D12RootSignature>& plDXBindingSetLayout::GetRootSignature() const
{
  return m_RootSignature;
}
