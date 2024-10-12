#pragma once
#include <RHI/RHIDLL.h>

#include <RHI/Program/Program.h>
#include <map>
#include <set>
#include <vector>

class PL_RHI_DLL plRHIProgramBase : public plRHIProgram
{
public:
  plRHIProgramBase(const std::vector<plSharedPtr<plRHIShader>>& shaders);

  bool HasShader(plRHIShaderType type) const override final;
  plSharedPtr<plRHIShader> GetShader(plRHIShaderType type) const override final;
  const std::vector<plSharedPtr<plRHIShader>>& GetShaders() const override final;
  const std::vector<plRHIBindKey>& GetBindings() const override final;
  const std::vector<plRHIEntryPoint>& GetEntryPoints() const override final;

protected:
  std::map<plRHIShaderType, plSharedPtr<plRHIShader>> m_ShadersByType;
  std::vector<plSharedPtr<plRHIShader>> m_Shaders;
  std::vector<plRHIBindKey> m_Bindings;
  std::vector<plRHIEntryPoint> m_EntryPoints;
};
