#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Swapchain/Swapchain.h>
#include <RHIVulkan/Resource/VKResource.h>
#include <memory>
#include <vector>

class plVKDevice;
class plVKCommandQueue;
class plRHICommandList;
class plRHIFence;

class plVKSwapchain : public plRHISwapchain
{
public:
  plVKSwapchain(plVKCommandQueue& commandQueue, plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync);
  ~plVKSwapchain();
  plRHIResourceFormat::Enum GetFormat() const override;
  plSharedPtr<plRHIResource> GetBackBuffer(plUInt32 buffer) override;
  plUInt32 NextImage(const plSharedPtr<plRHIFence>& fence, plUInt64 signalValue) override;
  void Present(const plSharedPtr<plRHIFence>& fence, plUInt64 waitValue) override;

private:
  plVKCommandQueue& m_CommandQueue;
  plVKDevice& m_Device;
  vk::UniqueSurfaceKHR m_Surface;
  vk::Format m_SwapchainColorFormat = vk::Format::eB8G8R8Unorm;
  vk::UniqueSwapchainKHR m_Swapchain;
  plDynamicArray<plSharedPtr<plRHIResource>> m_BackBuffers;
  plUInt32 m_FrameIndex = 0;
  vk::UniqueSemaphore m_ImageAvailableSemaphore;
  vk::UniqueSemaphore m_RenderingFinishedSemaphore;
  plSharedPtr<plRHICommandList> m_CommandList;
  plSharedPtr<plRHIFence> m_Fence;
};
