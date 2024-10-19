#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING 1

#include <RHIShaderCompilerHLSL/RHIShaderCompilerHLSLPCH.h>

#include <RHIShaderCompilerHLSL/DXCLoader.h>
#include <RHIShaderCompilerHLSL/DXILReflection.h>


#include <assert.h>

#include <dxc/include/dxc/DXIL/DxilConstants.h>
#include <dxc/include/dxc/DxilContainer/DxilContainer.h>
#include <dxc/include/dxc/DxilContainer/DxilRuntimeReflection.inl>

#include <algorithm>
#include <dia2.h>
#include <set>

plRHIShaderKind ConvertShaderKind(hlsl::DXIL::ShaderKind kind)
{
  switch (kind)
  {
    case hlsl::DXIL::ShaderKind::Pixel:
      return plRHIShaderKind::kPixel;
    case hlsl::DXIL::ShaderKind::Vertex:
      return plRHIShaderKind::kVertex;
    case hlsl::DXIL::ShaderKind::Geometry:
      return plRHIShaderKind::kGeometry;
    case hlsl::DXIL::ShaderKind::Compute:
      return plRHIShaderKind::kCompute;
    case hlsl::DXIL::ShaderKind::RayGeneration:
      return plRHIShaderKind::kRayGeneration;
    case hlsl::DXIL::ShaderKind::Intersection:
      return plRHIShaderKind::kIntersection;
    case hlsl::DXIL::ShaderKind::AnyHit:
      return plRHIShaderKind::kAnyHit;
    case hlsl::DXIL::ShaderKind::ClosestHit:
      return plRHIShaderKind::kClosestHit;
    case hlsl::DXIL::ShaderKind::Miss:
      return plRHIShaderKind::kMiss;
    case hlsl::DXIL::ShaderKind::Callable:
      return plRHIShaderKind::kCallable;
    case hlsl::DXIL::ShaderKind::Mesh:
      return plRHIShaderKind::kMesh;
    case hlsl::DXIL::ShaderKind::Amplification:
      return plRHIShaderKind::kAmplification;
  }
  assert(false);
  return plRHIShaderKind::kUnknown;
}

ComPtr<IDiaTable> FindTable(ComPtr<IDiaSession> session, const std::wstring& name)
{
  ComPtr<IDiaEnumTables> enum_tables;
  session->getEnumTables(&enum_tables);
  LONG count = 0;
  enum_tables->get_Count(&count);
  for (LONG i = 0; i < count; ++i)
  {
    ULONG fetched = 0;
    ComPtr<IDiaTable> table;
    enum_tables->Next(1, &table, &fetched);
    CComBSTR table_name;
    table->get_name(&table_name);
    if (table_name.m_str == name)
      return table;
  }
  return nullptr;
}

plString FindStrValue(ComPtr<IDiaTable> table, const std::wstring& name)
{
  LONG count = 0;
  table->get_Count(&count);
  for (LONG i = 0; i < count; ++i)
  {
    CComPtr<IUnknown> item;
    table->Item(i, &item);
    CComPtr<IDiaSymbol> symbol;
    if (FAILED(item.QueryInterface(&symbol)))
      continue;

    CComBSTR item_name;
    symbol->get_name(&item_name);
    if (!item_name || item_name.m_str != name)
      continue;



    VARIANT value = {};
    symbol->get_value(&value);
    if (value.vt == VT_BSTR)
      return plStringUtf8(value.bstrVal).GetData();
  }
  return "";
}

DXILReflection::DXILReflection(const void* data, size_t size)
{
  decltype(auto) dxc_support = GetDxcSupport(plRHIShaderBlobType::kDXIL);
  ComPtr<IDxcLibrary> library;
  dxc_support.CreateInstance(CLSID_DxcLibrary, library.GetAddressOf());
  ComPtr<IDxcBlobEncoding> source;
  PL_ASSERT_ALWAYS(library->CreateBlobWithEncodingOnHeapCopy(data, (plUInt32)size, CP_ACP, &source) == S_OK, "");
  ComPtr<IDxcContainerReflection> reflection;
  dxc_support.CreateInstance(CLSID_DxcContainerReflection, reflection.GetAddressOf());
  PL_ASSERT_ALWAYS(reflection->Load(source.Get()) == S_OK, "");

  ComPtr<IDxcBlob> pdb;
  uint32_t part_count = 0;
  PL_ASSERT_ALWAYS(reflection->GetPartCount(&part_count) == S_OK, "");
  for (uint32_t i = 0; i < part_count; ++i)
  {
    uint32_t kind = 0;
    PL_ASSERT_ALWAYS(reflection->GetPartKind(i, &kind) == S_OK, "");
    if (kind == hlsl::DxilFourCC::DFCC_RuntimeData)
    {
      ParseRuntimeData(reflection, i);
    }
    else if (kind == hlsl::DxilFourCC::DFCC_DXIL)
    {
      ComPtr<ID3D12ShaderReflection> shader_reflection;
      ComPtr<ID3D12LibraryReflection> library_reflection;
      if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&shader_reflection))))
      {
        ParseShaderReflection(shader_reflection);
      }
      else if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&library_reflection))))
      {
        m_is_library = true;
        ParseLibraryReflection(library_reflection);
      }
    }
    else if (kind == hlsl::DxilFourCC::DFCC_ShaderDebugInfoDXIL)
    {
      PL_ASSERT_ALWAYS(reflection->GetPartContent(i, &pdb) == S_OK, "");
    }
    else if (kind == hlsl::DxilFourCC::DFCC_FeatureInfo)
    {
      ComPtr<IDxcBlob> part;
      PL_ASSERT_ALWAYS(reflection->GetPartContent(i, &part) == S_OK, "");
      assert(part->GetBufferSize() == sizeof(DxilShaderFeatureInfo));
      auto feature_info = reinterpret_cast<DxilShaderFeatureInfo const*>(part->GetBufferPointer());
      if (feature_info->FeatureFlags & hlsl::DXIL::ShaderFeatureInfo_ResourceDescriptorHeapIndexing)
      {
        m_shader_feature_info.ResourceDescriptorHeapIndexing = true;
      }
      if (feature_info->FeatureFlags & hlsl::DXIL::ShaderFeatureInfo_SamplerDescriptorHeapIndexing)
      {
        m_shader_feature_info.SamplerDescriptorHeapIndexing = true;
      }
    }
  }

  if (pdb && !m_is_library)
  {
    ParseDebugInfo(dxc_support, pdb);
  }
}

const std::vector<plRHIEntryPoint>& DXILReflection::GetEntryPoints() const
{
  return m_entry_points;
}

const std::vector<plRHIResourceBindingDesc>& DXILReflection::GetBindings() const
{
  return m_bindings;
}

const std::vector<plRHIVariableLayout>& DXILReflection::GetVariableLayouts() const
{
  return m_layouts;
}

const std::vector<plRHIInputParameterDesc>& DXILReflection::GetInputParameters() const
{
  return m_input_parameters;
}

const std::vector<plRHIOutputParameterDesc>& DXILReflection::GetOutputParameters() const
{
  return m_output_parameters;
}

const plRHIShaderFeatureInfo& DXILReflection::GetShaderFeatureInfo() const
{
  return m_shader_feature_info;
}

void DXILReflection::ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx)
{
  ComPtr<IDxcBlob> part_blob;
  reflection->GetPartContent(idx, &part_blob);
  hlsl::RDAT::DxilRuntimeData context;
  context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
  hlsl::RDAT::FunctionTableReader* func_table_reader = context.GetFunctionTableReader();
  for (uint32_t j = 0; j < func_table_reader->GetNumFunctions(); ++j)
  {
    hlsl::RDAT::FunctionReader func_reader = func_table_reader->GetItem(j);
    auto kind = func_reader.GetShaderKind();
    m_entry_points.push_back({func_reader.GetUnmangledName(), ConvertShaderKind(kind), func_reader.GetPayloadSizeInBytes(), func_reader.GetAttributeSizeInBytes()});
  }
}

bool IsBufferDimension(D3D_SRV_DIMENSION dimension)
{
  switch (dimension)
  {
    case D3D_SRV_DIMENSION_BUFFER:
      return true;
    case D3D_SRV_DIMENSION_TEXTURE1D:
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
    case D3D_SRV_DIMENSION_TEXTURE2D:
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
    case D3D_SRV_DIMENSION_TEXTURE3D:
    case D3D_SRV_DIMENSION_TEXTURECUBE:
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
      return false;
    default:
      assert(false);
      return false;
  }
}

plRHIViewType GetViewType(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
  switch (bind_desc.Type)
  {
    case D3D_SIT_CBUFFER:
      return plRHIViewType::kConstantBuffer;
    case D3D_SIT_SAMPLER:
      return plRHIViewType::kSampler;
    case D3D_SIT_TEXTURE:
    {
      if (IsBufferDimension(bind_desc.Dimension))
      {
        return plRHIViewType::kBuffer;
      }
      else
      {
        return plRHIViewType::kTexture;
      }
    }
    case D3D_SIT_STRUCTURED:
      return plRHIViewType::kStructuredBuffer;
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
      return plRHIViewType::kAccelerationStructure;
    case D3D_SIT_UAV_RWSTRUCTURED:
      return plRHIViewType::kRWStructuredBuffer;
    case D3D_SIT_UAV_RWTYPED:
    {
      if (IsBufferDimension(bind_desc.Dimension))
      {
        return plRHIViewType::kRWBuffer;
      }
      else
      {
        return plRHIViewType::kRWTexture;
      }
    }
    default:
      assert(false);
      return plRHIViewType::kUnknown;
  }
}

plRHIViewDimension GetViewDimension(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
  switch (bind_desc.Dimension)
  {
    case D3D_SRV_DIMENSION_UNKNOWN:
      return plRHIViewDimension::kUnknown;
    case D3D_SRV_DIMENSION_BUFFER:
      return plRHIViewDimension::kBuffer;
    case D3D_SRV_DIMENSION_TEXTURE1D:
      return plRHIViewDimension::kTexture1D;
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
      return plRHIViewDimension::kTexture1DArray;
    case D3D_SRV_DIMENSION_TEXTURE2D:
      return plRHIViewDimension::kTexture2D;
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
      return plRHIViewDimension::kTexture2DArray;
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
      return plRHIViewDimension::kTexture2DMS;
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
      return plRHIViewDimension::kTexture2DMSArray;
    case D3D_SRV_DIMENSION_TEXTURE3D:
      return plRHIViewDimension::kTexture3D;
    case D3D_SRV_DIMENSION_TEXTURECUBE:
      return plRHIViewDimension::kTextureCube;
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
      return plRHIViewDimension::kTextureCubeArray;
    default:
      assert(false);
      return plRHIViewDimension::kUnknown;
  }
}

plRHIReturnType GetReturnType(plRHIViewType view_type, const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
  auto check_type = [&](plRHIReturnType return_type) {
    switch (view_type)
    {
      case plRHIViewType::kBuffer:
      case plRHIViewType::kRWBuffer:
      case plRHIViewType::kTexture:
      case plRHIViewType::kRWTexture:
        assert(return_type != plRHIReturnType::kUnknown);
        break;
      case plRHIViewType::kAccelerationStructure:
        return plRHIReturnType::kUnknown;
      default:
        assert(return_type == plRHIReturnType::kUnknown);
        break;
    }
    return return_type;
  };

  switch (bind_desc.ReturnType)
  {
    case D3D_RETURN_TYPE_FLOAT:
      return check_type(plRHIReturnType::kFloat);
    case D3D_RETURN_TYPE_UINT:
      return check_type(plRHIReturnType::kUint);
    case D3D_RETURN_TYPE_SINT:
      return check_type(plRHIReturnType::kInt);
    case D3D_RETURN_TYPE_DOUBLE:
      return check_type(plRHIReturnType::kDouble);
    default:
      return check_type(plRHIReturnType::kUnknown);
  }
}

template <typename T>
uint32_t GetStructureStride(plRHIViewType view_type, const D3D12_SHADER_INPUT_BIND_DESC& bind_desc, T* reflection)
{
  switch (view_type)
  {
    case plRHIViewType::kStructuredBuffer:
    case plRHIViewType::kRWStructuredBuffer:
      break;
    default:
      return 0;
  }

  auto get_buffer_stride = [&](const std::string& name) {
    ID3D12ShaderReflectionConstantBuffer* cbuffer = reflection->GetConstantBufferByName(name.c_str());
    if (cbuffer)
    {
      D3D12_SHADER_BUFFER_DESC cbuffer_desc = {};
      if (SUCCEEDED(cbuffer->GetDesc(&cbuffer_desc)))
      {
        return cbuffer_desc.Size;
      }
    }
    return 0u;
  };
  uint32_t stride = get_buffer_stride(bind_desc.Name);
  if (!stride)
  {
    stride = get_buffer_stride(std::string(bind_desc.Name) + "[0]");
  }
  assert(stride);
  return stride;
}

template <typename T>
plRHIResourceBindingDesc GetBindingDesc(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc, T* reflection)
{
  plRHIResourceBindingDesc desc = {};
  desc.name = bind_desc.Name;
  desc.type = GetViewType(bind_desc);
  desc.slot = bind_desc.BindPoint;
  desc.space = bind_desc.Space;
  desc.count = bind_desc.BindCount;
  if (desc.count == 0)
  {
    desc.count = std::numeric_limits<uint32_t>::max();
  }
  desc.dimension = GetViewDimension(bind_desc);
  desc.returnType = GetReturnType(desc.type, bind_desc);
  desc.structureStride = GetStructureStride(desc.type, bind_desc, reflection);
  return desc;
}

plRHIVariableLayout GetVariableLayout(const plString& name, uint32_t offset, uint32_t size, ID3D12ShaderReflectionType* variable_type)
{
  D3D12_SHADER_TYPE_DESC type_desc = {};
  variable_type->GetDesc(&type_desc);

  plRHIVariableLayout layout = {};
  layout.name = name;
  layout.offset = offset + type_desc.Offset;
  layout.size = size;
  layout.rows = type_desc.Rows;
  layout.columns = type_desc.Columns;
  layout.elements = type_desc.Elements;
  switch (type_desc.Type)
  {
    case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT:
      layout.type = plRHIVariableType::kFloat;
      break;
    case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_INT:
      layout.type = plRHIVariableType::kInt;
      break;
    case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_UINT:
      layout.type = plRHIVariableType::kUint;
      break;
    case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_BOOL:
      layout.type = plRHIVariableType::kBool;
      break;
    default:
      assert(false);
      break;
  }
  return layout;
}

template <typename ReflectionType>
plRHIVariableLayout GetBufferLayout(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc, ReflectionType* reflection)
{
  if (bind_desc.Type != D3D_SIT_CBUFFER)
  {
    return {};
  }
  ID3D12ShaderReflectionConstantBuffer* cbuffer = reflection->GetConstantBufferByName(bind_desc.Name);
  if (!cbuffer)
  {
    assert(false);
    return {};
  }

  D3D12_SHADER_BUFFER_DESC cbuffer_desc = {};
  cbuffer->GetDesc(&cbuffer_desc);

  plRHIVariableLayout layout = {};
  layout.name = bind_desc.Name;
  layout.type = plRHIVariableType::kStruct;
  layout.offset = 0;
  layout.size = cbuffer_desc.Size;
  for (UINT i = 0; i < cbuffer_desc.Variables; ++i)
  {
    ID3D12ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(i);
    D3D12_SHADER_VARIABLE_DESC variable_desc = {};
    variable->GetDesc(&variable_desc);
    layout.members.emplace_back(GetVariableLayout(variable_desc.Name, variable_desc.StartOffset, variable_desc.Size, variable->GetType()));
  }
  return layout;
}

template <typename T, typename U>
std::vector<plRHIResourceBindingDesc> ParseReflection(const T& desc, U* reflection)
{
  std::vector<plRHIResourceBindingDesc> res;
  res.reserve(desc.BoundResources);
  for (uint32_t i = 0; i < desc.BoundResources; ++i)
  {
    D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
    PL_ASSERT_ALWAYS(reflection->GetResourceBindingDesc(i, &bind_desc) == S_OK, "");
    res.emplace_back(GetBindingDesc(bind_desc, reflection));
  }
  return res;
}

template <typename T, typename U>
std::vector<plRHIVariableLayout> ParseLayout(const T& desc, U* reflection)
{
  std::vector<plRHIVariableLayout> res;
  res.reserve(desc.BoundResources);
  for (uint32_t i = 0; i < desc.BoundResources; ++i)
  {
    D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
    PL_ASSERT_ALWAYS(reflection->GetResourceBindingDesc(i, &bind_desc) == S_OK, "");
    res.emplace_back(GetBufferLayout(bind_desc, reflection));
  }
  return res;
}

std::vector<plRHIInputParameterDesc> ParseInputParameters(const D3D12_SHADER_DESC& desc, ComPtr<ID3D12ShaderReflection> shader_reflection)
{
  std::vector<plRHIInputParameterDesc> input_parameters;
  D3D12_SHADER_VERSION_TYPE type = static_cast<D3D12_SHADER_VERSION_TYPE>((desc.Version & 0xFFFF0000) >> 16);
  if (type != D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_VERTEX_SHADER)
  {
    return input_parameters;
  }
  for (uint32_t i = 0; i < desc.InputParameters; ++i)
  {
    D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
    PL_ASSERT_ALWAYS(shader_reflection->GetInputParameterDesc(i, &param_desc) == S_OK, "");
    decltype(auto) input = input_parameters.emplace_back();
    plStringBuilder semanticName(param_desc.SemanticName);
    if (param_desc.SemanticIndex)
    {
      semanticName.AppendFormat("{}", param_desc.SemanticIndex);
    }
    input.semanticName = semanticName.GetData();
    input.Location = i;
    if (param_desc.Mask == 1)
    {
      if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        input.Format = plRHIResourceFormat::R32_UINT; //gli::format::FORMAT_R32_UINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        input.Format = plRHIResourceFormat::R32_SINT; //gli::format::FORMAT_R32_SINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        input.Format = plRHIResourceFormat::R32_FLOAT; //gli::format::FORMAT_R32_SFLOAT_PACK32;
    }
    else if (param_desc.Mask <= 3)
    {
      if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        input.Format = plRHIResourceFormat::R32G32_UINT; //gli::format::FORMAT_RG32_UINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        input.Format = plRHIResourceFormat::R32G32_SINT; //gli::format::FORMAT_RG32_SINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        input.Format = plRHIResourceFormat::R32G32_FLOAT; //gli::format::FORMAT_RG32_SFLOAT_PACK32;
    }
    else if (param_desc.Mask <= 7)
    {
      if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        input.Format = plRHIResourceFormat::R32G32B32_UINT; //gli::format::FORMAT_RGB32_UINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        input.Format = plRHIResourceFormat::R32G32B32_SINT; //gli::format::FORMAT_RGB32_SINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        input.Format = plRHIResourceFormat::R32G32B32_FLOAT; //gli::format::FORMAT_RGB32_SFLOAT_PACK32;
    }
    else if (param_desc.Mask <= 15)
    {
      if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        input.Format = plRHIResourceFormat::R32G32B32A32_UINT; //gli::format::FORMAT_RGBA32_UINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        input.Format = plRHIResourceFormat::R32G32B32A32_SINT; //gli::format::FORMAT_RGBA32_SINT_PACK32;
      else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        input.Format = plRHIResourceFormat::R32G32B32A32_FLOAT; //gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
    }
  }
  return input_parameters;
}

std::vector<plRHIOutputParameterDesc> ParseOutputParameters(const D3D12_SHADER_DESC& desc, ComPtr<ID3D12ShaderReflection> shader_reflection)
{
  std::vector<plRHIOutputParameterDesc> output_parameters;
  D3D12_SHADER_VERSION_TYPE type = static_cast<D3D12_SHADER_VERSION_TYPE>((desc.Version & 0xFFFF0000) >> 16);
  if (type != D3D12_SHADER_VERSION_TYPE::D3D12_SHVER_PIXEL_SHADER)
  {
    return output_parameters;
  }
  for (uint32_t i = 0; i < desc.OutputParameters; ++i)
  {
    D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
    PL_ASSERT_ALWAYS(shader_reflection->GetOutputParameterDesc(i, &param_desc) == S_OK, "");
    assert(param_desc.SemanticName == std::string("SV_TARGET"));
    assert(param_desc.SystemValueType == D3D_NAME_TARGET);
    assert(param_desc.SemanticIndex == param_desc.Register);
    decltype(auto) output = output_parameters.emplace_back();
    output.slot = param_desc.Register;
  }
  return output_parameters;
}

void DXILReflection::ParseShaderReflection(ComPtr<ID3D12ShaderReflection> shader_reflection)
{
  D3D12_SHADER_DESC desc = {};
  PL_ASSERT_ALWAYS(shader_reflection->GetDesc(&desc) == S_OK, "");
  hlsl::DXIL::ShaderKind kind = hlsl::GetVersionShaderType(desc.Version);
  m_entry_points.push_back({"", ConvertShaderKind(kind)});
  m_bindings = ParseReflection(desc, shader_reflection.Get());
  m_layouts = ParseLayout(desc, shader_reflection.Get());
  assert(m_bindings.size() == m_layouts.size());
  m_input_parameters = ParseInputParameters(desc, shader_reflection);
  m_output_parameters = ParseOutputParameters(desc, shader_reflection);
}

void DXILReflection::ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> library_reflection)
{
  D3D12_LIBRARY_DESC library_desc = {};
  PL_ASSERT_ALWAYS(library_reflection->GetDesc(&library_desc) == S_OK, "");
  std::map<plString, size_t> exist;
  for (uint32_t i = 0; i < library_desc.FunctionCount; ++i)
  {
    ID3D12FunctionReflection* function_reflection = library_reflection->GetFunctionByIndex(i);
    D3D12_FUNCTION_DESC function_desc = {};
    PL_ASSERT_ALWAYS(function_reflection->GetDesc(&function_desc) == S_OK, "");
    auto function_bindings = ParseReflection(function_desc, function_reflection);
    auto function_layouts = ParseLayout(function_desc, function_reflection);
    assert(function_bindings.size() == function_layouts.size());
    for (size_t i = 0; i < function_bindings.size(); ++i)
    {
      auto it = exist.find(function_bindings[i].name);
      if (it == exist.end())
      {
        exist[function_bindings[i].name] = m_bindings.size();
        m_bindings.emplace_back(function_bindings[i]);
        m_layouts.emplace_back(function_layouts[i]);
      }
      else
      {
        assert(function_bindings[i] == m_bindings[it->second]);
        assert(function_layouts[i] == m_layouts[it->second]);
      }
    }
  }
}

void DXILReflection::ParseDebugInfo(dxc::DxcDllSupport& dxc_support, ComPtr<IDxcBlob> pdb)
{
  ComPtr<IDxcLibrary> library;
  PL_ASSERT_ALWAYS(dxc_support.CreateInstance(CLSID_DxcLibrary, library.GetAddressOf()) == S_OK, "");
  ComPtr<IStream> stream;
  PL_ASSERT_ALWAYS(library->CreateStreamFromBlobReadOnly(pdb.Get(), &stream) == S_OK, "");

  ComPtr<IDiaDataSource> dia;
  PL_ASSERT_ALWAYS(dxc_support.CreateInstance(CLSID_DxcDiaDataSource, dia.GetAddressOf()) == S_OK, "");
  PL_ASSERT_ALWAYS(dia->loadDataFromIStream(stream.Get()) == S_OK, "");
  ComPtr<IDiaSession> session;
  PL_ASSERT_ALWAYS(dia->openSession(&session) == S_OK, "");

  ComPtr<IDiaTable> symbols_table = FindTable(session, L"Symbols");
  assert(m_entry_points.size() == 1);
  m_entry_points.front().name = FindStrValue(symbols_table, L"hlslEntry");
}
