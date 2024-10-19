#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Fence/VKTimelineSemaphore.h>
#include <RHIVulkan/Device/VKDevice.h>

plVKTimelineSemaphore::plVKTimelineSemaphore(plVKDevice& device, plUInt64 initialValue)
    : m_Device(device)
{
    vk::SemaphoreTypeCreateInfo timelineCreateInfo = {};
    timelineCreateInfo.initialValue = initialValue;
    timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
    vk::SemaphoreCreateInfo createInfo;
    createInfo.pNext = &timelineCreateInfo;
    m_TimelineSemaphore = device.GetDevice().createSemaphoreUnique(createInfo);
}

plUInt64 plVKTimelineSemaphore::GetCompletedValue()
{
    return m_Device.GetDevice().getSemaphoreCounterValueKHR(m_TimelineSemaphore.get());
}

void plVKTimelineSemaphore::Wait(plUInt64 value)
{
    vk::SemaphoreWaitInfo waitInfo = {};
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &m_TimelineSemaphore.get();
    waitInfo.pValues = &value;
    vk::Result result = m_Device.GetDevice().waitSemaphoresKHR(waitInfo, UINT64_MAX);
}

void plVKTimelineSemaphore::Signal(plUInt64 value)
{
    vk::SemaphoreSignalInfo signalInfo = {};
    signalInfo.semaphore = m_TimelineSemaphore.get();
    signalInfo.value = value;
    m_Device.GetDevice().signalSemaphoreKHR(signalInfo);
}

const vk::Semaphore& plVKTimelineSemaphore::GetFence() const
{
    return m_TimelineSemaphore.get();
}
