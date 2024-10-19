#pragma once
#include <RHI/Instance/Instance.h>
#include <dxgi1_4.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXInstance : public plRHIInstance
{
public:
    plDXInstance();
  plDynamicArray<plSharedPtr<plRHIAdapter>> EnumerateAdapters() override;
    ComPtr<IDXGIFactory4> GetFactory();

private:
    ComPtr<IDXGIFactory4> m_DxgiFactory;
};
