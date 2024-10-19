#include <RHIDX12/Adapter/DXAdapter.h>
#include <RHIDX12/CommandQueue/DXCommandQueue.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Instance/DXInstance.h>
#include <RHIDX12/Resource/DXResource.h>
#include <RHIDX12/Swapchain/DXSwapchain.h>
#include <RHIDX12/Utilities/DXUtility.h>

plDXSwapchain::plDXSwapchain(plDXCommandQueue& commandQueue, plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync)
  : m_CommandQueue(commandQueue)
  , m_VSync(vsync)
{
  plDXInstance& instance = commandQueue.GetDevice().GetAdapter().GetInstance();
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = plDXUtils::ToDXGIFormat(GetFormat());
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = frameCount;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

  ComPtr<IDXGISwapChain1> tmpSwapChain;
  PL_ASSERT_ALWAYS(instance.GetFactory()->CreateSwapChainForHwnd(commandQueue.GetQueue().Get(), reinterpret_cast<HWND>(window), &swapChainDesc, nullptr, nullptr, &tmpSwapChain) == S_OK, "");
  PL_ASSERT_ALWAYS(instance.GetFactory()->MakeWindowAssociation(reinterpret_cast<HWND>(window), DXGI_MWA_NO_WINDOW_CHANGES) == S_OK, "");
  tmpSwapChain.As(&m_SwapChain);

  for (plUInt32 i = 0; i < frameCount; ++i)
  {
    plSharedPtr<plDXResource> res = PL_DEFAULT_NEW(plDXResource, commandQueue.GetDevice());
    ComPtr<ID3D12Resource> backBuffer;
    PL_ASSERT_ALWAYS(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)) == S_OK, "");
    res->Format = GetFormat();
    res->SetInitialState(plRHIResourceState::kPresent);
    res->resource = backBuffer;
    res->desc = backBuffer->GetDesc();
    res->m_IsBackBuffer = true;
    m_BackBuffers.PushBack(res);
  }
}

plRHIResourceFormat::Enum plDXSwapchain::GetFormat() const
{
  return plRHIResourceFormat::R8G8B8A8_UNORM;
}

plSharedPtr<plRHIResource> plDXSwapchain::GetBackBuffer(plUInt32 buffer)
{
  return m_BackBuffers[buffer];
}

plUInt32 plDXSwapchain::NextImage(const plSharedPtr<plRHIFence>& fence, plUInt64 signalValue)
{
  plUInt32 frameIndex = m_SwapChain->GetCurrentBackBufferIndex();
  m_CommandQueue.Signal(fence, signalValue);
  return frameIndex;
}

void plDXSwapchain::Present(const plSharedPtr<plRHIFence>& fence, plUInt64 waitValue)
{
  m_CommandQueue.Wait(fence, waitValue);
  if (m_VSync)
  {
    PL_ASSERT_ALWAYS(m_SwapChain->Present(1, 0) == S_OK, "");
  }
  else
  {
    PL_ASSERT_ALWAYS(m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING) == S_OK, "");
  }
}
