#pragma once
#include <RHI/Resource/Resource.h>
#include <RHI/Swapchain/Swapchain.h>
#include <cstdint>
#include <directx/d3d12.h>
#include <dxgi1_4.h>
#include <vector>
#include <wrl.h>
using namespace Microsoft::WRL;

class plDXCommandQueue;

class plDXSwapchain : public plRHISwapchain
{
public:
  plDXSwapchain(plDXCommandQueue& commandQueue, plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync);
  plRHIResourceFormat::Enum GetFormat() const override;
  plSharedPtr<plRHIResource> GetBackBuffer(plUInt32 buffer) override;
  plUInt32 NextImage(const plSharedPtr<plRHIFence>& fence, plUInt64 signalValue) override;
  void Present(const plSharedPtr<plRHIFence>& fence, plUInt64 waitValue) override;

private:
  plDXCommandQueue& m_CommandQueue;
  bool m_VSync;
  ComPtr<IDXGISwapChain3> m_SwapChain;
  plDynamicArray<plSharedPtr<plRHIResource>> m_BackBuffers;
};
