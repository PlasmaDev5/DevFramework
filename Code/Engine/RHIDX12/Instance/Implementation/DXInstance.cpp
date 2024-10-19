#include <RHIDX12/RHIDX12PCH.h>

#include <RHIDX12/Adapter/DXAdapter.h>
#include <Foundation/Configuration/Startup.h>
#include <RHIDX12/Instance/DXInstance.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <Foundation/IO/OSFile.h>
#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <filesystem>

plSharedPtr<plRHIInstance> CreateDXInstance()
{
  return PL_DEFAULT_NEW(plDXInstance);
}

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RHIDX12, InstanceFactory)

ON_CORESYSTEMS_STARTUP
{
  plRHIInstanceFactory::RegisterCreatorFunc(plRHIApiType::kDX12, &CreateDXInstance);
}

ON_CORESYSTEMS_SHUTDOWN
{
  plRHIInstanceFactory::UnregisterCreatorFunc(plRHIApiType::kDX12);
}

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool EnableAgilitySDKIfExist(plUInt32 version, const plString& path)
{
  plStringBuilder d3d12Core(plOSFile::GetApplicationDirectory(), "/", path, "/D3D12Core.dll");
  if (!plOSFile::ExistsFile(d3d12Core))
  {
    return false;
  }

  HMODULE d3d12 = GetModuleHandleA("d3d12.dll");
  assert(d3d12);
  auto D3D12GetInterfacePfn = (PFN_D3D12_GET_INTERFACE)GetProcAddress(d3d12, "D3D12GetInterface");
  if (!D3D12GetInterfacePfn)
  {
    return false;
  }

  ComPtr<ID3D12SDKConfiguration> sdkConfiguration;
  if (FAILED(D3D12GetInterfacePfn(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdkConfiguration))))
  {
    return false;
  }
  if (FAILED(sdkConfiguration->SetSDKVersion(version, path)))
  {
    return false;
  }
  return true;
}

#ifdef AGILITY_SDK_REQUIRED
#  define EXPORT_AGILITY_SDK extern "C" _declspec(dllexport) extern
#else
#  define EXPORT_AGILITY_SDK
#endif

EXPORT_AGILITY_SDK const UINT D3D12SDKVersion = 4;
EXPORT_AGILITY_SDK const char* D3D12SDKPath = u8".\\D3D12\\";

#ifndef AGILITY_SDK_REQUIRED
static bool optionalAgilitySdk = EnableAgilitySDKIfExist(D3D12SDKVersion, D3D12SDKPath);
#endif

plDXInstance::plDXInstance()
{
#if 0
    static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
        0x76f5573e,
        0xf13a,
        0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
    };
    PL_ASSERT_ALWAYS(D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr) == S_OK, "");
#endif

  plUInt32 flags = 0;
  static const bool debugEnabled = IsDebuggerPresent();
  if (debugEnabled)
  {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
      debugController->EnableDebugLayer();
    }
    /*ComPtr<ID3D12Debug1> debugController1;
        debugCcontroller.As(&debugController1);
        if (debugController1)
            debugController1->SetEnableSynchronizedCommandQueueValidation(true);*/
    flags = DXGI_CREATE_FACTORY_DEBUG;
  }

  PL_ASSERT_ALWAYS(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_DxgiFactory)) == S_OK, "");
}

plDynamicArray<plSharedPtr<plRHIAdapter>> plDXInstance::EnumerateAdapters()
{
  plDynamicArray<plSharedPtr<plRHIAdapter>> adapters;

  ComPtr<IDXGIFactory6> dxgiFactory6;
  m_DxgiFactory.As(&dxgiFactory6);

  auto NextAdapted = [&](plUInt32 adapterIndex, ComPtr<IDXGIAdapter1>& adapter) {
    if (dxgiFactory6)
      return dxgiFactory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
    else
      return m_DxgiFactory->EnumAdapters1(adapterIndex, &adapter);
  };

  ComPtr<IDXGIAdapter1> adapter;
  //plUInt32 gpuIndex = 0;
  for (plUInt32 adapterIndex = 0; DXGI_ERROR_NOT_FOUND != NextAdapted(adapterIndex, adapter); ++adapterIndex)
  {
    DXGI_ADAPTER_DESC1 desc = {};
    adapter->GetDesc1(&desc);
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      continue;

    adapters.PushBack(PL_DEFAULT_NEW(plDXAdapter,*this, adapter));
  }
  return adapters;
}

ComPtr<IDXGIFactory4> plDXInstance::GetFactory()
{
  return m_DxgiFactory;
}
