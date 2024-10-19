#pragma once
#include <RHI/Adapter/Adapter.h>
#include <dxgi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXInstance;

class plDXAdapter : public plRHIAdapter
{
public:
    plDXAdapter(plDXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter);
    const plString& GetName() const override;
    plSharedPtr<plRHIDevice> CreateDevice() override;
    plDXInstance& GetInstance();
    ComPtr<IDXGIAdapter1> GetAdapter();

private:
    plDXInstance& m_Instance;
    ComPtr<IDXGIAdapter1> m_Adapter;
    plString m_Name;
};
