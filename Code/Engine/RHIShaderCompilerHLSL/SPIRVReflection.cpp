#include <RHIShaderCompilerHLSL/SPIRVReflection.h>

plRHIShaderKind ConvertShaderKind(spv::ExecutionModel execution_model)
{
  switch (execution_model)
  {
    case spv::ExecutionModel::ExecutionModelVertex:
      return plRHIShaderKind::kVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
      return plRHIShaderKind::kPixel;
    case spv::ExecutionModel::ExecutionModelGeometry:
      return plRHIShaderKind::kGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
      return plRHIShaderKind::kCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
      return plRHIShaderKind::kRayGeneration;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
      return plRHIShaderKind::kIntersection;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
      return plRHIShaderKind::kAnyHit;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
      return plRHIShaderKind::kClosestHit;
    case spv::ExecutionModel::ExecutionModelMissNV:
      return plRHIShaderKind::kMiss;
    case spv::ExecutionModel::ExecutionModelCallableNV:
      return plRHIShaderKind::kCallable;
    case spv::ExecutionModel::ExecutionModelTaskNV:
      return plRHIShaderKind::kAmplification;
    case spv::ExecutionModel::ExecutionModelMeshNV:
      return plRHIShaderKind::kMesh;
  }
  assert(false);
  return plRHIShaderKind::kUnknown;
}

std::vector<plRHIInputParameterDesc> ParseInputParameters(const spirv_cross::Compiler& compiler)
{
  plString s;
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();
  std::vector<plRHIInputParameterDesc> input_parameters;
  for (const auto& resource : resources.stage_inputs)
  {
    decltype(auto) input = input_parameters.emplace_back();
    input.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);
    input.semanticName = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE).data();
    if (!input.semanticName.IsEmpty() && input.semanticName.EndsWith("0"))
    {
      input.semanticName = input.semanticName.GetSubString(0, input.semanticName.GetCharacterCount() - 1);
    }
    decltype(auto) type = compiler.get_type(resource.base_type_id);
    if (type.basetype == spirv_cross::SPIRType::Float)
    {
      if (type.vecsize == 1)
      {
        input.Format = plRHIResourceFormat::R32_FLOAT; // gli::format::FORMAT_R32_SFLOAT_PACK32;
      }
      else if (type.vecsize == 2)
      {
        input.Format = plRHIResourceFormat::R32G32_FLOAT; //gli::format::FORMAT_RG32_SFLOAT_PACK32;
      }
      else if (type.vecsize == 3)
      {
        input.Format = plRHIResourceFormat::R32G32B32_FLOAT; //gli::format::FORMAT_RGB32_SFLOAT_PACK32;
      }
      else if (type.vecsize == 4)
      {
        input.Format = plRHIResourceFormat::R32G32B32A32_FLOAT; // gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
      }
    }
    else if (type.basetype == spirv_cross::SPIRType::UInt)
    {
      if (type.vecsize == 1)
      {
        input.Format = plRHIResourceFormat::R32_UINT; // gli::format::FORMAT_R32_UINT_PACK32;
      }
      else if (type.vecsize == 2)
      {
        input.Format = plRHIResourceFormat::R32G32_UINT; // gli::format::FORMAT_RG32_UINT_PACK32;
      }
      else if (type.vecsize == 3)
      {
        input.Format = plRHIResourceFormat::R32G32B32_UINT; // gli::format::FORMAT_RGB32_UINT_PACK32;
      }
      else if (type.vecsize == 4)
      {
        input.Format = plRHIResourceFormat::R32G32B32A32_UINT; // gli::format::FORMAT_RGBA32_UINT_PACK32;
      }
    }
    else if (type.basetype == spirv_cross::SPIRType::Int)
    {
      if (type.vecsize == 1)
      {
        input.Format = plRHIResourceFormat::R32_SINT; // gli::format::FORMAT_R32_SINT_PACK32;
      }
      else if (type.vecsize == 2)
      {
        input.Format = plRHIResourceFormat::R32G32_SINT; // gli::format::FORMAT_RG32_SINT_PACK32;
      }
      else if (type.vecsize == 3)
      {
        input.Format = plRHIResourceFormat::R32G32B32_SINT; // gli::format::FORMAT_RGB32_SINT_PACK32;
      }
      else if (type.vecsize == 4)
      {
        input.Format = plRHIResourceFormat::R32G32B32A32_SINT; // gli::format::FORMAT_RGBA32_SINT_PACK32;
      }
    }
  }
  return input_parameters;
}

std::vector<plRHIOutputParameterDesc> ParseOutputParameters(const spirv_cross::Compiler& compiler)
{
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();
  std::vector<plRHIOutputParameterDesc> output_parameters;
  for (const auto& resource : resources.stage_outputs)
  {
    decltype(auto) output = output_parameters.emplace_back();
    output.slot = compiler.get_decoration(resource.id, spv::DecorationLocation);
  }
  return output_parameters;
}

bool IsBufferDimension(spv::Dim dimension)
{
  switch (dimension)
  {
    case spv::Dim::DimBuffer:
      return true;
    case spv::Dim::Dim1D:
    case spv::Dim::Dim2D:
    case spv::Dim::Dim3D:
    case spv::Dim::DimCube:
      return false;
    default:
      assert(false);
      return false;
  }
}

plRHIViewType GetViewType(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type, uint32_t resource_id)
{
  switch (type.basetype)
  {
    case spirv_cross::SPIRType::AccelerationStructure:
    {
      return plRHIViewType::kAccelerationStructure;
    }
    case spirv_cross::SPIRType::SampledImage:
    case spirv_cross::SPIRType::Image:
    {
      bool is_readonly = (type.image.sampled != 2);
      if (IsBufferDimension(type.image.dim))
      {
        if (is_readonly)
          return plRHIViewType::kBuffer;
        else
          return plRHIViewType::kRWBuffer;
      }
      else
      {
        if (is_readonly)
          return plRHIViewType::kTexture;
        else
          return plRHIViewType::kRWTexture;
      }
    }
    case spirv_cross::SPIRType::Sampler:
    {
      return plRHIViewType::kSampler;
    }
    case spirv_cross::SPIRType::Struct:
    {
      if (type.storage == spv::StorageClassStorageBuffer)
      {
        spirv_cross::Bitset flags = compiler.get_buffer_block_flags(resource_id);
        bool is_readonly = flags.get(spv::DecorationNonWritable);
        if (is_readonly)
        {
          return plRHIViewType::kStructuredBuffer;
        }
        else
        {
          return plRHIViewType::kRWStructuredBuffer;
        }
      }
      else if (type.storage == spv::StorageClassPushConstant || type.storage == spv::StorageClassUniform)
      {
        return plRHIViewType::kConstantBuffer;
      }
      assert(false);
      return plRHIViewType::kUnknown;
    }
    default:
      assert(false);
      return plRHIViewType::kUnknown;
  }
}

plRHIViewDimension GetDimension(spv::Dim dim, const spirv_cross::SPIRType& resource_type)
{
  switch (dim)
  {
    case spv::Dim::Dim1D:
    {
      if (resource_type.image.arrayed)
        return plRHIViewDimension::kTexture1DArray;
      else
        return plRHIViewDimension::kTexture1D;
    }
    case spv::Dim::Dim2D:
    {
      if (resource_type.image.arrayed)
        return plRHIViewDimension::kTexture2DArray;
      else
        return plRHIViewDimension::kTexture2D;
    }
    case spv::Dim::Dim3D:
    {
      return plRHIViewDimension::kTexture3D;
    }
    case spv::Dim::DimCube:
    {
      if (resource_type.image.arrayed)
        return plRHIViewDimension::kTextureCubeArray;
      else
        return plRHIViewDimension::kTextureCube;
    }
    case spv::Dim::DimBuffer:
    {
      return plRHIViewDimension::kBuffer;
    }
    default:
      assert(false);
      return plRHIViewDimension::kUnknown;
  }
}

plRHIViewDimension GetViewDimension(const spirv_cross::SPIRType& resource_type)
{
  if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Image)
  {
    return GetDimension(resource_type.image.dim, resource_type);
  }
  else if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
  {
    return plRHIViewDimension::kBuffer;
  }
  else
  {
    return plRHIViewDimension::kUnknown;
  }
}

plRHIReturnType GetReturnType(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::SPIRType& resource_type)
{
  if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Image)
  {
    decltype(auto) image_type = compiler.get_type(resource_type.image.type);
    switch (image_type.basetype)
    {
      case spirv_cross::SPIRType::BaseType::Float:
        return plRHIReturnType::kFloat;
      case spirv_cross::SPIRType::BaseType::UInt:
        return plRHIReturnType::kUint;
      case spirv_cross::SPIRType::BaseType::Int:
        return plRHIReturnType::kInt;
      case spirv_cross::SPIRType::BaseType::Double:
        return plRHIReturnType::kDouble;
    }
    assert(false);
  }
  return plRHIReturnType::kUnknown;
}

plRHIResourceBindingDesc GetBindingDesc(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::Resource& resource)
{
  plRHIResourceBindingDesc desc = {};
  decltype(auto) type = compiler.get_type(resource.type_id);
  desc.name = compiler.get_name(resource.id).data();
  desc.type = GetViewType(compiler, type, resource.id);
  desc.slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
  desc.space = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
  desc.count = 1;
  if (!type.array.empty() && type.array.front() == 0)
  {
    desc.count = std::numeric_limits<uint32_t>::max();
  }
  desc.dimension = GetViewDimension(type);
  desc.returnType = GetReturnType(compiler, type);
  switch (desc.type)
  {
    case plRHIViewType::kStructuredBuffer:
    case plRHIViewType::kRWStructuredBuffer:
    {
      bool is_block = compiler.get_decoration_bitset(type.self).get(spv::DecorationBlock) ||
                      compiler.get_decoration_bitset(type.self).get(spv::DecorationBufferBlock);
      bool is_sized_block = is_block && (compiler.get_storage_class(resource.id) == spv::StorageClassUniform ||
                                          compiler.get_storage_class(resource.id) == spv::StorageClassUniformConstant ||
                                          compiler.get_storage_class(resource.id) == spv::StorageClassStorageBuffer);
      assert(is_sized_block);
      decltype(auto) base_type = compiler.get_type(resource.base_type_id);
      desc.structureStride = (plUInt32)(compiler.get_declared_struct_size_runtime_array(base_type, 1) - compiler.get_declared_struct_size_runtime_array(base_type, 0));
      assert(desc.structureStride);
      break;
    }
  }
  return desc;
}

plRHIVariableLayout GetBufferMemberLayout(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::TypeID& type_id)
{
  decltype(auto) type = compiler.get_type(type_id);
  plRHIVariableLayout layout = {};
  layout.columns = type.vecsize;
  layout.rows = type.columns;
  if (!type.array.empty())
  {
    assert(type.array.size() == 1);
    layout.elements = type.array.front();
  }
  switch (type.basetype)
  {
    case spirv_cross::SPIRType::BaseType::Float:
      layout.type = plRHIVariableType::kFloat;
      break;
    case spirv_cross::SPIRType::BaseType::Int:
      layout.type = plRHIVariableType::kInt;
      break;
    case spirv_cross::SPIRType::BaseType::UInt:
      layout.type = plRHIVariableType::kUint;
      break;
    case spirv_cross::SPIRType::BaseType::Boolean:
      layout.type = plRHIVariableType::kBool;
      break;
    default:
      assert(false);
      break;
  }
  return layout;
}

plRHIVariableLayout GetBufferLayout(plRHIViewType view_type, const spirv_cross::CompilerHLSL& compiler, const spirv_cross::Resource& resource)
{
  if (view_type != plRHIViewType::kConstantBuffer)
  {
    return {};
  }

  plRHIVariableLayout layout = {};
  decltype(auto) type = compiler.get_type(resource.base_type_id);
  layout.name = compiler.get_name(resource.id).data();
  layout.size = (plUInt32)compiler.get_declared_struct_size(type);
  assert(type.basetype == spirv_cross::SPIRType::BaseType::Struct);
  for (plUInt32 i = 0; i < (plUInt32)type.member_types.size(); ++i)
  {
    auto& member = layout.members.emplace_back(GetBufferMemberLayout(compiler, type.member_types[i]));
    member.name = compiler.get_member_name(resource.base_type_id, i).data();
    member.offset = compiler.type_struct_member_offset(type, i);
    member.size = (plUInt32)compiler.get_declared_struct_member_size(type, i);
  }
  return layout;
}

void ParseBindings(const spirv_cross::CompilerHLSL& compiler, std::vector<plRHIResourceBindingDesc>& bindings, std::vector<plRHIVariableLayout>& layouts)
{
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();
  auto enumerate_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {
    for (const auto& resource : resources)
    {
      bindings.emplace_back(GetBindingDesc(compiler, resource));
      layouts.emplace_back(GetBufferLayout(bindings.back().type, compiler, resource));
    }
  };
  enumerate_resources(resources.uniform_buffers);
  enumerate_resources(resources.storage_buffers);
  enumerate_resources(resources.storage_images);
  enumerate_resources(resources.separate_images);
  enumerate_resources(resources.separate_samplers);
  enumerate_resources(resources.atomic_counters);
  enumerate_resources(resources.acceleration_structures);
}

SPIRVReflection::SPIRVReflection(const void* data, size_t size)
  : m_Blob((const uint32_t*)data, (const uint32_t*)data + size / sizeof(uint32_t))
{
  spirv_cross::CompilerHLSL compiler(m_Blob);
  auto entry_points = compiler.get_entry_points_and_stages();
  for (const auto& entry_point : entry_points)
  {
    m_EntryPoints.push_back({entry_point.name.c_str(), ConvertShaderKind(entry_point.execution_model)});
  }
  ParseBindings(compiler, m_Bindings, m_Layouts);
  m_InputParameters = ParseInputParameters(compiler);
  m_OutputParameters = ParseOutputParameters(compiler);
}

const std::vector<plRHIEntryPoint>& SPIRVReflection::GetEntryPoints() const
{
  return m_EntryPoints;
}

const std::vector<plRHIResourceBindingDesc>& SPIRVReflection::GetBindings() const
{
  return m_Bindings;
}

const std::vector<plRHIVariableLayout>& SPIRVReflection::GetVariableLayouts() const
{
  return m_Layouts;
}

const std::vector<plRHIInputParameterDesc>& SPIRVReflection::GetInputParameters() const
{
  return m_InputParameters;
}

const std::vector<plRHIOutputParameterDesc>& SPIRVReflection::GetOutputParameters() const
{
  return m_OutputParameters;
}

const plRHIShaderFeatureInfo& SPIRVReflection::GetShaderFeatureInfo() const
{
  return m_ShaderFeatureInfo;
}
