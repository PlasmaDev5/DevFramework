#pragma once
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <spirv_hlsl.hpp>
#include <string>
#include <vector>

class SPIRVReflection : public plRHIShaderReflection
{
public:
  SPIRVReflection(const void* data, size_t size);
  const std::vector<plRHIEntryPoint>& GetEntryPoints() const override;
  const std::vector<plRHIResourceBindingDesc>& GetBindings() const override;
  const std::vector<plRHIVariableLayout>& GetVariableLayouts() const override;
  const std::vector<plRHIInputParameterDesc>& GetInputParameters() const override;
  const std::vector<plRHIOutputParameterDesc>& GetOutputParameters() const override;
  const plRHIShaderFeatureInfo& GetShaderFeatureInfo() const override;

private:
  std::vector<uint32_t> m_Blob;
  std::vector<plRHIEntryPoint> m_EntryPoints;
  std::vector<plRHIResourceBindingDesc> m_Bindings;
  std::vector<plRHIVariableLayout> m_Layouts;
  std::vector<plRHIInputParameterDesc> m_InputParameters;
  std::vector<plRHIOutputParameterDesc> m_OutputParameters;
  plRHIShaderFeatureInfo m_ShaderFeatureInfo = {};
};
