#pragma once
#include <RHI/ShaderReflection/ShaderReflection.h>
#include <RHIShaderCompilerHLSL/DXCLoader.h>
#include <d3d12shader.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXILReflection : public plRHIShaderReflection
{
public:
  DXILReflection(const void* data, size_t size);
  const std::vector<plRHIEntryPoint>& GetEntryPoints() const override;
  const std::vector<plRHIResourceBindingDesc>& GetBindings() const override;
  const std::vector<plRHIVariableLayout>& GetVariableLayouts() const override;
  const std::vector<plRHIInputParameterDesc>& GetInputParameters() const override;
  const std::vector<plRHIOutputParameterDesc>& GetOutputParameters() const override;
  const plRHIShaderFeatureInfo& GetShaderFeatureInfo() const override;

private:
  void ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx);
  void ParseShaderReflection(ComPtr<ID3D12ShaderReflection> shader_reflection);
  void ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> library_reflection);
  void ParseDebugInfo(dxc::DxcDllSupport& dxc_support, ComPtr<IDxcBlob> pdb);

  bool m_is_library = false;
  std::vector<plRHIEntryPoint> m_entry_points;
  std::vector<plRHIResourceBindingDesc> m_bindings;
  std::vector<plRHIVariableLayout> m_layouts;
  std::vector<plRHIInputParameterDesc> m_input_parameters;
  std::vector<plRHIOutputParameterDesc> m_output_parameters;
  plRHIShaderFeatureInfo m_shader_feature_info = {};
};
