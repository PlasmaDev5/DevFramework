#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Adapter/VKAdapter.h>
#include <RHIVulkan/CommandQueue/VKCommandQueue.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Fence/VKTimelineSemaphore.h>
#include <RHIVulkan/Instance/VKInstance.h>
#include <RHIVulkan/Resource/VKResource.h>
#include <RHIVulkan/Swapchain/VKSwapchain.h>
#include <RHIVulkan/Utilities/VKUtility.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

PL_DEFINE_AS_POD_TYPE(vk::Image);
PL_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);
PL_DEFINE_AS_POD_TYPE(vk::SurfaceFormatKHR);

plVKSwapchain::plVKSwapchain(plVKCommandQueue& commandQueue, plRHIWindow window, plUInt32 width, plUInt32 height, plUInt32 frameCount, bool vsync)
  : m_CommandQueue(commandQueue)
  , m_Device(commandQueue.GetDevice())
{
  plVKAdapter& adapter = m_Device.GetAdapter();
  plVKInstance& instance = adapter.GetInstance();

  vk::Win32SurfaceCreateInfoKHR surfaceDesc = {};
  surfaceDesc.hinstance = GetModuleHandle(nullptr);
  surfaceDesc.hwnd = reinterpret_cast<HWND>(window);
  m_Surface = instance.GetInstance().createWin32SurfaceKHRUnique(surfaceDesc);

  plUInt32 surfaceFormatsCount = 0;
  vk::Result result = adapter.GetPhysicalDevice().getSurfaceFormatsKHR(m_Surface.get(), &surfaceFormatsCount, nullptr);

  plDynamicArray<vk::SurfaceFormatKHR> surfaceFormats;
  surfaceFormats.SetCountUninitialized(surfaceFormatsCount);
  result = adapter.GetPhysicalDevice().getSurfaceFormatsKHR(m_Surface.get(), &surfaceFormatsCount, surfaceFormats.GetData());

  PL_ASSERT_ALWAYS(!surfaceFormats.IsEmpty(), "");

  if (surfaceFormats[0].format != vk::Format::eUndefined)
    m_SwapchainColorFormat = surfaceFormats[0].format;

  vk::ColorSpaceKHR color_space = surfaceFormats[0].colorSpace;

  vk::SurfaceCapabilitiesKHR surfaceCapabilities = {};
  PL_ASSERT_ALWAYS(adapter.GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_Surface.get(), &surfaceCapabilities) == vk::Result::eSuccess, "");

  PL_ASSERT_ALWAYS(surfaceCapabilities.currentExtent.width == width, "");
  PL_ASSERT_ALWAYS(surfaceCapabilities.currentExtent.height == height, "");

  vk::Bool32 isSupportedSurface = VK_FALSE;
  result = adapter.GetPhysicalDevice().getSurfaceSupportKHR(commandQueue.GetQueueFamilyIndex(), m_Surface.get(), &isSupportedSurface);
  PL_ASSERT_ALWAYS(isSupportedSurface, "");

  plUInt32 presentModesCount = 0;
  result = adapter.GetPhysicalDevice().getSurfacePresentModesKHR(m_Surface.get(), &presentModesCount, nullptr);

  plDynamicArray<vk::PresentModeKHR> presentModes;
  presentModes.SetCountUninitialized(presentModesCount);
  result = adapter.GetPhysicalDevice().getSurfacePresentModesKHR(m_Surface.get(), &presentModesCount, presentModes.GetData());

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.surface = m_Surface.get();
  swapChainCreateInfo.minImageCount = frameCount;
  swapChainCreateInfo.imageFormat = m_SwapchainColorFormat;
  swapChainCreateInfo.imageColorSpace = color_space;
  swapChainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  if (vsync)
  {
    if (presentModes.Contains(vk::PresentModeKHR::eFifoRelaxed))
      swapChainCreateInfo.presentMode = vk::PresentModeKHR::eFifoRelaxed;
    else
      swapChainCreateInfo.presentMode = vk::PresentModeKHR::eFifo;
  }
  else
  {
    if (presentModes.Contains(vk::PresentModeKHR::eMailbox))
      swapChainCreateInfo.presentMode = vk::PresentModeKHR::eMailbox;
    else
      swapChainCreateInfo.presentMode = vk::PresentModeKHR::eImmediate;
  }
  swapChainCreateInfo.clipped = true;

  m_Swapchain = m_Device.GetDevice().createSwapchainKHRUnique(swapChainCreateInfo);

  plUInt32 imageCount = 0;
  result = m_Device.GetDevice().getSwapchainImagesKHR(m_Swapchain.get(), &imageCount, nullptr);

  plDynamicArray<vk::Image> images;
  images.SetCountUninitialized(imageCount);
  result = m_Device.GetDevice().getSwapchainImagesKHR(m_Swapchain.get(), &imageCount, images.GetData());

  m_CommandList = m_Device.CreateCommandList(plRHICommandListType::kGraphics);
  for (plUInt32 i = 0; i < frameCount; ++i)
  {
    plSharedPtr<plVKResource> res = PL_DEFAULT_NEW(plVKResource,m_Device);
    res->Format = GetFormat();
    res->image.res = images[i];
    res->image.format = m_SwapchainColorFormat;
    res->image.size = vk::Extent2D(1u * width, 1u * height);
    res->ResourceType = plRHIResourceType::kTexture;
    res->m_IsBackBuffer = true;
    m_CommandList->ResourceBarrier({{res, plRHIResourceState::kUndefined, plRHIResourceState::kPresent}});
    res->SetInitialState(plRHIResourceState::kPresent);
    m_BackBuffers.PushBack(res);
  }
  m_CommandList->Close();

  vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
  m_ImageAvailableSemaphore = m_Device.GetDevice().createSemaphoreUnique(semaphoreCreateInfo);
  m_RenderingFinishedSemaphore = m_Device.GetDevice().createSemaphoreUnique(semaphoreCreateInfo);
  m_Fence = m_Device.CreateFence(0);
  commandQueue.ExecuteCommandLists({m_CommandList});
  commandQueue.Signal(m_Fence, 1);
}

plVKSwapchain::~plVKSwapchain()
{
  m_Fence->Wait(1);
}

plRHIResourceFormat::Enum plVKSwapchain::GetFormat() const
{
  return plVKUtils::ToEngineFormat(m_SwapchainColorFormat);
}

plSharedPtr<plRHIResource> plVKSwapchain::GetBackBuffer(plUInt32 buffer)
{
  return m_BackBuffers[buffer];
}

plUInt32 plVKSwapchain::NextImage(const plSharedPtr<plRHIFence>& fence, plUInt64 signalValue)
{
  vk::Result result = m_Device.GetDevice().acquireNextImageKHR(m_Swapchain.get(), UINT64_MAX, m_ImageAvailableSemaphore.get(), nullptr, &m_FrameIndex);

  plSharedPtr<plVKTimelineSemaphore> vkFence = fence.Downcast<plVKTimelineSemaphore>();
  constexpr plUInt64 tmp = plMath::MaxValue<plUInt64>();
  vk::TimelineSemaphoreSubmitInfo timelineInfo = {};
  timelineInfo.waitSemaphoreValueCount = 1;
  timelineInfo.pWaitSemaphoreValues = &tmp;
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &signalValue;
  vk::SubmitInfo signalSubmitInfo = {};
  signalSubmitInfo.pNext = &timelineInfo;
  signalSubmitInfo.waitSemaphoreCount = 1;
  signalSubmitInfo.pWaitSemaphores = &m_ImageAvailableSemaphore.get();
  vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
  signalSubmitInfo.pWaitDstStageMask = &waitDstStageMask;
  signalSubmitInfo.signalSemaphoreCount = 1;
  signalSubmitInfo.pSignalSemaphores = &vkFence->GetFence();
  result = m_CommandQueue.GetQueue().submit(1, &signalSubmitInfo, {});

  return m_FrameIndex;
}

void plVKSwapchain::Present(const plSharedPtr<plRHIFence>& fence, plUInt64 waitValue)
{
  plSharedPtr<plVKTimelineSemaphore> vkFence = fence.Downcast<plVKTimelineSemaphore>();
  constexpr plUInt64 tmp = plMath::MaxValue<plUInt64>();
  vk::TimelineSemaphoreSubmitInfo timelineInfo = {};
  timelineInfo.waitSemaphoreValueCount = 1;
  timelineInfo.pWaitSemaphoreValues = &waitValue;
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &tmp;
  vk::SubmitInfo signalSubmitInfo = {};
  signalSubmitInfo.pNext = &timelineInfo;
  signalSubmitInfo.waitSemaphoreCount = 1;
  signalSubmitInfo.pWaitSemaphores = &vkFence->GetFence();
  vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
  signalSubmitInfo.pWaitDstStageMask = &waitDstStageMask;
  signalSubmitInfo.signalSemaphoreCount = 1;
  signalSubmitInfo.pSignalSemaphores = &m_RenderingFinishedSemaphore.get();
  vk::Result result = m_CommandQueue.GetQueue().submit(1, &signalSubmitInfo, {});

  vk::PresentInfoKHR presentInfo = {};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_Swapchain.get();
  presentInfo.pImageIndices = &m_FrameIndex;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_RenderingFinishedSemaphore.get();
  result = m_CommandQueue.GetQueue().presentKHR(presentInfo);
}
