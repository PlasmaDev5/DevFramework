#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <memory>

class PL_RHI_DLL plRHIShader : public plRefCounted
{
public:
  virtual ~plRHIShader() = default;
  virtual plRHIShaderType GetType() const = 0;
  virtual const plDynamicArray<plUInt8>& GetBlob() const = 0;
  virtual plUInt64 GetId(const plString& entryPoint) const = 0;
  virtual const plRHIBindKey& GetBindKey(const plString& name) const = 0;
  virtual const std::vector<plRHIResourceBindingDesc>& GetResourceBindings() const = 0;
  virtual const plRHIResourceBindingDesc& GetResourceBinding(const plRHIBindKey& bindKey) const = 0;
  virtual const std::vector<plRHIInputLayoutDesc>& GetInputLayouts() const = 0;
  virtual plUInt32 GetInputLayoutLocation(const plString& semanticName) const = 0;
  virtual const std::vector<plRHIBindKey>& GetBindings() const = 0;
  virtual const plSharedPtr<plRHIShaderReflection>& GetReflection() const = 0;
};
