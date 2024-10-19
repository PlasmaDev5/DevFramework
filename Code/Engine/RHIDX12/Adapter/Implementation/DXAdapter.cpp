#include <RHIDX12/Adapter/DXAdapter.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>

plDXAdapter::plDXAdapter(plDXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter)
    : m_Instance(instance)
    , m_Adapter(adapter)
{
    DXGI_ADAPTER_DESC desc = {};
    adapter->GetDesc(&desc);
    m_Name = plStringUtf8(desc.Description).GetData();
}

const plString& plDXAdapter::GetName() const
{
    return m_Name;
}

plSharedPtr<plRHIDevice> plDXAdapter::CreateDevice()
{
    return PL_DEFAULT_NEW(plDXDevice, *this);
}

plDXInstance& plDXAdapter::GetInstance()
{
    return m_Instance;
}

ComPtr<IDXGIAdapter1> plDXAdapter::GetAdapter()
{
    return m_Adapter;
}
