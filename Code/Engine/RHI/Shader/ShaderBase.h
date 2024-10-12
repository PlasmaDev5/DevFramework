#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Shader/Shader.h>
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <map>

class PL_RHI_DLL plRHIShaderBase : public plRHIShader
{
public:
  plRHIShaderBase(const plRHIShaderDesc& desc, plDynamicArray<plUInt8> byteCode, plSharedPtr<plRHIShaderReflection> reflection, plRHIShaderBlobType blobType);
  plRHIShaderType GetType() const override;
  const plDynamicArray<plUInt8>& GetBlob() const override;
  plUInt64 GetId(const plString& entryPoint) const override;
  const plRHIBindKey& GetBindKey(const plString& name) const override;
  const std::vector<plRHIResourceBindingDesc>& GetResourceBindings() const override;
  const plRHIResourceBindingDesc& GetResourceBinding(const plRHIBindKey& bind_key) const override;
  const std::vector<plRHIInputLayoutDesc>& GetInputLayouts() const override;
  plUInt32 GetInputLayoutLocation(const plString& semanticName) const override;
  const std::vector<plRHIBindKey>& GetBindings() const override;
  const plSharedPtr<plRHIShaderReflection>& GetReflection() const override;

protected:
  plRHIShaderType m_ShaderType;
  plRHIShaderBlobType m_BlobType;
  plDynamicArray<plUInt8> m_Blob;
  std::map<plString, plUInt64> m_Ids;
  std::vector<plRHIResourceBindingDesc> m_Bindings;
  std::vector<plRHIBindKey> m_BindingKeys;
  std::map<plRHIBindKey, size_t> m_Mapping;
  std::map<plString, plRHIBindKey> m_BindKeys;
  std::vector<plRHIInputLayoutDesc> m_InputLayoutDescs;
  std::map<plString, plUInt32> m_Locations;
  plSharedPtr<plRHIShaderReflection> m_Reflection;
};
