#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Fence/DXFence.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>

plDXFence::plDXFence(plDXDevice& device, plUInt64 initialValue)
  : m_Device(device)
{
  PL_ASSERT_ALWAYS(device.GetDevice()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)) == S_OK, "");
  m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

plUInt64 plDXFence::GetCompletedValue()
{
  return m_pFence->GetCompletedValue();
}

void plDXFence::Wait(plUInt64 value)
{
  if (GetCompletedValue() < value)
  {
    PL_ASSERT_ALWAYS(m_pFence->SetEventOnCompletion(value, m_hFenceEvent) == S_OK, "");
    WaitForSingleObjectEx(m_hFenceEvent, INFINITE, FALSE);
  }
}

void plDXFence::Signal(plUInt64 value)
{
  m_pFence->Signal(value);
}

ComPtr<ID3D12Fence> plDXFence::GetFence()
{
  return m_pFence;
}
