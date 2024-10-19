#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/CommandList/VKCommandList.h>
#include <RHIVulkan/CommandQueue/VKCommandQueue.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/Fence/VKTimelineSemaphore.h>

plVKCommandQueue::plVKCommandQueue(plVKDevice& device, plRHICommandListType type, plUInt32 queueFamilyIndex)
  : m_Device(device)
  , m_QueueFamilyIndex(queueFamilyIndex)
{
  m_Queue = m_Device.GetDevice().getQueue(m_QueueFamilyIndex, 0);
}

void plVKCommandQueue::Wait(const plSharedPtr<plRHIFence>& fence, plUInt64 value)
{
  plSharedPtr<plVKTimelineSemaphore> vkFence = fence.Downcast<plVKTimelineSemaphore>();
  vk::TimelineSemaphoreSubmitInfo timelineInfo = {};
  timelineInfo.waitSemaphoreValueCount = 1;
  timelineInfo.pWaitSemaphoreValues = &value;

  vk::SubmitInfo signalSubmitInfo = {};
  signalSubmitInfo.pNext = &timelineInfo;
  signalSubmitInfo.waitSemaphoreCount = 1;
  signalSubmitInfo.pWaitSemaphores = &vkFence->GetFence();
  vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eAllCommands;
  signalSubmitInfo.pWaitDstStageMask = &waitDstStageMask;
  vk::Result res = m_Queue.submit(1, &signalSubmitInfo, {});
}

void plVKCommandQueue::Signal(const plSharedPtr<plRHIFence>& fence, plUInt64 value)
{
  plSharedPtr<plVKTimelineSemaphore> vkFence = fence.Downcast<plVKTimelineSemaphore>();
  vk::TimelineSemaphoreSubmitInfo timelineInfo = {};
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &value;

  vk::SubmitInfo signalSubmitInfo = {};
  signalSubmitInfo.pNext = &timelineInfo;
  signalSubmitInfo.signalSemaphoreCount = 1;
  signalSubmitInfo.pSignalSemaphores = &vkFence->GetFence();
  vk::Result result = m_Queue.submit(1, &signalSubmitInfo, {});
}

void plVKCommandQueue::ExecuteCommandLists(const std::vector<plSharedPtr<plRHICommandList>>& commandLists)
{
  plDynamicArray<vk::CommandBuffer> vkCommandLists;
  for (auto& commandList : commandLists)
  {
    if (!commandList)
      continue;
    plSharedPtr<plVKCommandList> vkCommandList = commandList.Downcast<plVKCommandList>();
    vkCommandLists.PushBack(vkCommandList->GetCommandList());
  }

  vk::SubmitInfo submitInfo = {};
  submitInfo.commandBufferCount = vkCommandLists.GetCount();
  submitInfo.pCommandBuffers = vkCommandLists.GetData();

  vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eAllCommands;
  submitInfo.pWaitDstStageMask = &waitDstStageMask;

  vk::Result result = m_Queue.submit(1, &submitInfo, {});
}

plVKDevice& plVKCommandQueue::GetDevice()
{
  return m_Device;
}

plUInt32 plVKCommandQueue::GetQueueFamilyIndex()
{
  return m_QueueFamilyIndex;
}

vk::Queue plVKCommandQueue::GetQueue()
{
  return m_Queue;
}
