                                                    #include <RHI/Shader/Shader.h>
#include <RHIDX12/BindingSetLayout/DXBindingSetLayout.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Pipeline/DXRayTracingPipeline.h>
#include <RHIDX12/Program/DXProgram.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <RHIDX12/View/DXView.h>
#include <d3d12shader.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

plDXRayTracingPipeline::plDXRayTracingPipeline(plDXDevice& device, const plRHIRayTracingPipelineDesc& desc)
  : m_Device(device)
  , m_Desc(desc)
{
  decltype(auto) shaders = m_Desc.program->GetShaders();
  plSharedPtr<plDXBindingSetLayout> dxLayout = m_Desc.layout.Downcast<plDXBindingSetLayout>();
  m_RootSignature = dxLayout->GetRootSignature();

  CD3DX12_STATE_OBJECT_DESC subobjects(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

  decltype(auto) entryPoints = m_Desc.program->GetEntryPoints();
  for (const auto& shader : shaders)
  {
    decltype(auto) library = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    decltype(auto) blob = shader->GetBlob();
    D3D12_SHADER_BYTECODE byte = {blob.GetData(), blob.GetCount()};
    library->SetDXILLibrary(&byte);
    for (const auto& entryPoint : shader->GetReflection()->GetEntryPoints())
    {
      plUInt64 shaderId = shader->GetId(entryPoint.name);
      plString shaderName = entryPoint.name;
      if (m_ShaderNames.Contains(shaderName))
      {
        plStringBuilder renamed(shaderName, "_renamed_");
        renamed.AppendFormat("{}", shaderId);
        plString newShaderName = GenerateUniqueName(renamed);
        library->DefineExport(plStringWChar(newShaderName).GetData(), plStringWChar(shaderName).GetData());
        shaderName = newShaderName;
      }
      else
      {
        library->DefineExport(plStringWChar(shaderName).GetData());
      }
      m_ShaderNames.Insert(shaderName);
      m_ShaderIds[shaderId] = shaderName;
    }
  }

  size_t hitGroupCount = 0;
  for (size_t i = 0; i < m_Desc.groups.size(); ++i)
  {
    if (m_Desc.groups[i].type == plRHIRayTracingShaderGroupType::kGeneral)
    {
      m_GroupNames[i] = m_ShaderIds[m_Desc.groups[i].general];
      continue;
    }
    decltype(auto) hitGroup = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    plStringBuilder hitGroupName("hit_group_");
    hitGroupName.AppendFormat("{}", hitGroupCount++);
    plString name = GenerateUniqueName(hitGroupName);
    hitGroup->SetHitGroupExport(plStringWChar(name).GetData());
    switch (m_Desc.groups[i].type)
    {
      case plRHIRayTracingShaderGroupType::kTrianglesHitGroup:
        if (m_Desc.groups[i].anyHit)
        {
          hitGroup->SetAnyHitShaderImport(plStringWChar(m_ShaderIds[m_Desc.groups[i].anyHit]).GetData());
        }
        if (m_Desc.groups[i].closestHit)
        {
          hitGroup->SetClosestHitShaderImport(plStringWChar(m_ShaderIds[m_Desc.groups[i].closestHit]).GetData());
        }
        break;
      case plRHIRayTracingShaderGroupType::kProceduralHitGroup:
        if (m_Desc.groups[i].intersection)
        {
          hitGroup->SetIntersectionShaderImport(plStringWChar(m_ShaderIds[m_Desc.groups[i].intersection]).GetData());
        }
        break;
    }
    m_GroupNames[i] = name;
  }

  decltype(auto) globalRootSignature = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
  globalRootSignature->SetRootSignature(m_RootSignature.Get());

  decltype(auto) shaderConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();

  plUInt32 maxPayloadSize = 0;
  plUInt32 maxAttributeSize = 0;
  for (size_t i = 0; i < entryPoints.size(); ++i)
  {
    maxPayloadSize = plMath::Max(maxPayloadSize, entryPoints[i].payloadSize);
    maxAttributeSize = plMath::Max(maxPayloadSize, entryPoints[i].attributeSize);
  }
  shaderConfig->Config(maxPayloadSize, maxAttributeSize);

  decltype(auto) pipelineConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
  pipelineConfig->Config(1);

  ComPtr<ID3D12Device5> device5;
  m_Device.GetDevice().As(&device5);
  PL_ASSERT_ALWAYS(device5->CreateStateObject(subobjects, IID_PPV_ARGS(&m_PipelineState)) == S_OK, "");
  m_PipelineState.As(&m_StateOjbectProps);
}

plString plDXRayTracingPipeline::GenerateUniqueName(plString name)
{
  plStringBuilder builder(name);
  static plUInt64 id = 0;
  while (m_ShaderNames.Contains(builder))
  {
    builder.AppendFormat("_{}", ++id);
  }
  return builder.GetData();
}

plRHIPipelineType plDXRayTracingPipeline::GetPipelineType() const
{
  return plRHIPipelineType::kRayTracing;
}

const plRHIRayTracingPipelineDesc& plDXRayTracingPipeline::GetDesc() const
{
  return m_Desc;
}

const ComPtr<ID3D12StateObject>& plDXRayTracingPipeline::GetPipeline() const
{
  return m_PipelineState;
}

const ComPtr<ID3D12RootSignature>& plDXRayTracingPipeline::GetRootSignature() const
{
  return m_RootSignature;
}

plDynamicArray<plUInt8> plDXRayTracingPipeline::GetRayTracingShaderGroupHandles(plUInt32 firstGroup, plUInt32 groupCount) const
{
  plDynamicArray<plUInt8> shaderHandlesStorage;
  shaderHandlesStorage.SetCountUninitialized(groupCount * m_Device.GetShaderGroupHandleSize());
  for (plUInt32 i = 0; i < groupCount; ++i)
  {
    memcpy(shaderHandlesStorage.GetData() + i * m_Device.GetShaderGroupHandleSize(), m_StateOjbectProps->GetShaderIdentifier(plStringWChar((*m_GroupNames.GetValue(i + firstGroup))).GetData()), m_Device.GetShaderGroupHandleSize());
  }
  return shaderHandlesStorage;
}
