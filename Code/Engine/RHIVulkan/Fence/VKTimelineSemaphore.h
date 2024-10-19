#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Fence/Fence.h>

class plVKDevice;

class plVKTimelineSemaphore : public plRHIFence
{
public:
    plVKTimelineSemaphore(plVKDevice& device, plUInt64 initialValue);
    plUInt64 GetCompletedValue() override;
    void Wait(plUInt64 value) override;
    void Signal(plUInt64 value) override;

    const vk::Semaphore& GetFence() const;

private:
    plVKDevice& m_Device;
    vk::UniqueSemaphore m_TimelineSemaphore;
};
