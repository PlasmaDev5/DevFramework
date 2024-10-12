#include <RHI/Shader/ShaderBase.h>

static plUInt64 GenId()
{
  static plUInt64 id = 0;
  return ++id;
}

plRHIShaderBase::plRHIShaderBase(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection, plRHIShaderBlobType blobType)
    : m_ShaderType(desc.type)
    , m_BlobType(blobType)
{
  m_Blob = byteCode;
  m_Reflection = reflection;
  m_Bindings = m_Reflection->GetBindings();
  for (plUInt32 i = 0; i < m_Bindings.size(); ++i)
  {
    plRHIBindKey bindKey = {m_ShaderType, m_Bindings[i].type, m_Bindings[i].slot, m_Bindings[i].space, m_Bindings[i].count};
    m_BindKeys[m_Bindings[i].name] = bindKey;
    m_Mapping[bindKey] = i;
    m_BindingKeys.emplace_back(bindKey);
  }

  decltype(auto) inputParameters = m_Reflection->GetInputParameters();
  for (plUInt32 i = 0; i < inputParameters.size(); ++i)
  {
    decltype(auto) layout = m_InputLayoutDescs.emplace_back();
    layout.slot = i;
    layout.semanticName = inputParameters[i].semanticName;
    layout.format = inputParameters[i].Format;
    layout.stride = plRHIResourceFormat::GetFormatStride(layout.format);
    m_Locations[inputParameters[i].semanticName] = inputParameters[i].Location;
  }

  for (const auto& entryPoint : m_Reflection->GetEntryPoints())
  {
    m_Ids.emplace(entryPoint.name, GenId());
  }
}

plRHIShaderType plRHIShaderBase::GetType() const
{
    return m_ShaderType;
}

const plDynamicArray<plUInt8>& plRHIShaderBase::GetBlob() const
{
    return m_Blob;
}

plUInt64 plRHIShaderBase::GetId(const plString& entryPoint) const
{
    return m_Ids.at(entryPoint);
}

const plRHIBindKey& plRHIShaderBase::GetBindKey(const plString& name) const
{
    return m_BindKeys.at(name);
}

const std::vector<plRHIResourceBindingDesc>& plRHIShaderBase::GetResourceBindings() const
{
    return m_Bindings;
}

const plRHIResourceBindingDesc& plRHIShaderBase::GetResourceBinding(const plRHIBindKey& bind_key) const
{
    return m_Bindings.at(m_Mapping.at(bind_key));
}

const std::vector<plRHIInputLayoutDesc>& plRHIShaderBase::GetInputLayouts() const
{
    return m_InputLayoutDescs;
}

plUInt32 plRHIShaderBase::GetInputLayoutLocation(const plString& semanticName) const
{
    return m_Locations.at(semanticName);
}

const std::vector<plRHIBindKey>& plRHIShaderBase::GetBindings() const
{
    return m_BindingKeys;
}

const plSharedPtr<plRHIShaderReflection>& plRHIShaderBase::GetReflection() const
{
    return m_Reflection;
}
