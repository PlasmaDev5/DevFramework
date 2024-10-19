#include <RHIShaderCompilerHLSL/RHIShaderCompilerHLSLPCH.h>

#include <RHIShaderCompilerHLSL/Compiler.h>
#include <RHIShaderCompilerHLSL/DXCLoader.h>
#include <Foundation/Logging/Log.h>
#include <cassert>
#include <d3dcompiler.h>
#include <deque>
#include <iostream>
#include <vector>
#include <wrl.h>
using namespace Microsoft::WRL;

static plString GetShaderTarget(plRHIShaderType type, const plString& model)
{
  switch (type)
  {
    case plRHIShaderType ::kPixel:
      return plStringBuilder("ps_", model);
    case plRHIShaderType ::kVertex:
      return plStringBuilder("vs_", model);
    case plRHIShaderType ::kGeometry:
      return plStringBuilder("gs_", model);
    case plRHIShaderType ::kCompute:
      return plStringBuilder("cs_", model);
    case plRHIShaderType ::kAmplification:
      return plStringBuilder("as_", model);
    case plRHIShaderType ::kMesh:
      return plStringBuilder("ms_", model);
    case plRHIShaderType ::kLibrary:
      return plStringBuilder("lib_", model);
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

class IncludeHandler : public IDxcIncludeHandler
{
public:
  IncludeHandler(ComPtr<IDxcLibrary> library, const std::wstring& basePath)
    : m_Library(library)
    , m_BasePath(basePath)
  {
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override { return (HRESULT)E_NOTIMPL; }
  ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG)E_NOTIMPL; }
  ULONG STDMETHODCALLTYPE Release() override { return (ULONG)E_NOTIMPL; }

  HRESULT STDMETHODCALLTYPE LoadSource(
    _In_ LPCWSTR pFilename,
    _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
  {
    std::wstring path = m_BasePath + pFilename;
    ComPtr<IDxcBlobEncoding> source;
    HRESULT hr = m_Library->CreateBlobFromFile(
      path.c_str(),
      nullptr,
      &source);
    if (SUCCEEDED(hr) && ppIncludeSource)
      *ppIncludeSource = source.Detach();
    return hr;
  }

private:
  ComPtr<IDxcLibrary> m_Library;
  const std::wstring& m_BasePath;
};

plDynamicArray<plUInt8> Compile(plRHIShaderDesc shader, plRHIShaderBlobType blobType)
{
  decltype(auto) dxcSupport = GetDxcSupport(blobType);

  std::wstring shaderPath = plStringWChar(shader.shaderPath).GetData();
  std::wstring shaderDir = shaderPath.substr(0, shaderPath.find_last_of(L"\\/") + 1);

  ComPtr<IDxcLibrary> library;
  dxcSupport.CreateInstance(CLSID_DxcLibrary, library.GetAddressOf());
  ComPtr<IDxcBlobEncoding> source;

  PL_ASSERT_ALWAYS(library->CreateBlobFromFile(
                     shaderPath.c_str(),
                     nullptr,
                     &source) == S_OK,
    "");

  std::wstring target = plStringWChar(GetShaderTarget(shader.type, shader.model)).GetData();
  std::wstring entrypoint = plStringWChar(shader.entrypoint).GetData();
  std::vector<std::pair<std::wstring, std::wstring>> definesStore;
  std::vector<DxcDefine> defines;
  for (const auto& define : shader.define)
  {
    definesStore.emplace_back(plStringWChar(define.first).GetData(), plStringWChar(define.second).GetData());
    defines.push_back({definesStore.back().first.c_str(), definesStore.back().second.c_str()});
  }

  std::vector<LPCWSTR> arguments;
  std::deque<std::wstring> dynamicArguments;
  arguments.push_back(L"/Zi");
  arguments.push_back(L"/Qembed_debug");
  uint32_t space = 0;
  if (blobType == plRHIShaderBlobType::kSPIRV)
  {
    arguments.emplace_back(L"-spirv");
    arguments.emplace_back(L"-fspv-target-env=vulkan1.2");
    arguments.emplace_back(L"-fspv-extension=KHR");
    arguments.emplace_back(L"-fspv-extension=SPV_NV_mesh_shader");
    arguments.emplace_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
    arguments.emplace_back(L"-fspv-extension=SPV_EXT_shader_viewport_index_layer");
    arguments.emplace_back(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1");
    arguments.emplace_back(L"-fspv-extension=SPV_GOOGLE_user_type");
    arguments.emplace_back(L"-fvk-use-dx-layout");
    arguments.emplace_back(L"-fspv-reflect");
    space = static_cast<uint32_t>(shader.type);
  }

  arguments.emplace_back(L"-auto-binding-space");
  dynamicArguments.emplace_back(std::to_wstring(space));
  arguments.emplace_back(dynamicArguments.back().c_str());

  ComPtr<IDxcOperationResult> result;
  IncludeHandler include_handler(library, shaderDir);
  ComPtr<IDxcCompiler> compiler;
  dxcSupport.CreateInstance(CLSID_DxcCompiler, compiler.GetAddressOf());
  PL_ASSERT_ALWAYS(compiler->Compile(
                     source.Get(),
                     L"main.hlsl",
                     entrypoint.c_str(),
                     target.c_str(),
                     arguments.data(), static_cast<UINT32>(arguments.size()),
                     defines.data(), static_cast<UINT32>(defines.size()),
                     &include_handler,
                     &result) == S_OK,
    "");

  HRESULT hr = {};
  result->GetStatus(&hr);
  plDynamicArray<plUInt8> blob;
  if (SUCCEEDED(hr))
  {
    ComPtr<IDxcBlob> dxcBlob;
    PL_ASSERT_ALWAYS(result->GetResult(&dxcBlob) == S_OK, "");
    blob.SetCountUninitialized((plUInt32)dxcBlob->GetBufferSize());
    plMemoryUtils::Copy(blob.GetData(), (plUInt8*)dxcBlob->GetBufferPointer(), blob.GetCount());
  }
  else
  {
    ComPtr<IDxcBlobEncoding> errors;
    result->GetErrorBuffer(&errors);
    plLog::Error(reinterpret_cast<char*>(errors->GetBufferPointer()));
  }
  return blob;
}
