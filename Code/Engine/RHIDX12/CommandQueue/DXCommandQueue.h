#pragma once
#include <RHI/CommandQueue/CommandQueue.h>
#include <RHIDX12/RHIDX12DLL.h>
#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXDevice;

class plDXCommandQueue : public plRHICommandQueue
{
public:
  plDXCommandQueue(plDXDevice& device, plRHICommandListType type);
  void Wait(const plSharedPtr<plRHIFence>& fence, plUInt64 value) override;
  void Signal(const plSharedPtr<plRHIFence>& fence, plUInt64 value) override;
  void ExecuteCommandLists(const std::vector<plSharedPtr<plRHICommandList>>& commandLists) override;

  plDXDevice& GetDevice();
  ComPtr<ID3D12CommandQueue> GetQueue();

private:
  plDXDevice& m_Device;
  ComPtr<ID3D12CommandQueue> m_CommandQueue;
};
