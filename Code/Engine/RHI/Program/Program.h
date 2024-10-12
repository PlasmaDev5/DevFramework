#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/BindingSet/BindingSet.h>
#include <RHI/Instance/BaseTypes.h>
#include <RHI/Instance/QueryInterface.h>
#include <RHI/Shader/Shader.h>
#include <memory>

class PL_RHI_DLL plRHIProgram : public plRefCounted
{
public:
  virtual ~plRHIProgram() = default;
  virtual bool HasShader(plRHIShaderType type) const = 0;
  virtual plSharedPtr<plRHIShader> GetShader(plRHIShaderType type) const = 0;
  virtual const std::vector<plSharedPtr<plRHIShader>>& GetShaders() const = 0;
  virtual const std::vector<plRHIBindKey>& GetBindings() const = 0;
  virtual const std::vector<plRHIEntryPoint>& GetEntryPoints() const = 0;
};
