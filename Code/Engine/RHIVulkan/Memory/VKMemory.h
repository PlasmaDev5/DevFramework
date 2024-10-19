#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/Memory/Memory.h>
#include <RHI/Instance/BaseTypes.h>

class plVKDevice;

class plVKMemory : public plRHIMemory
{
public:
  plVKMemory(plVKDevice& device, plUInt64 size, plRHIMemoryType memory_type, plUInt32 memory_type_bits, const vk::MemoryDedicatedAllocateInfoKHR* dedicated_allocate_info);
  plRHIMemoryType GetMemoryType() const override;
    vk::DeviceMemory GetMemory() const;

private:
    plRHIMemoryType m_memory_type;
    vk::UniqueDeviceMemory m_memory;
};
