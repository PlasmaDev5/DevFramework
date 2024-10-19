#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/Memory/VKMemory.h>
#include <RHIVulkan/Device/VKDevice.h>

plVKMemory::plVKMemory(plVKDevice& device, plUInt64 size, plRHIMemoryType memory_type, plUInt32 memory_type_bits, const vk::MemoryDedicatedAllocateInfoKHR* dedicated_allocate_info)
    : m_memory_type(memory_type)
{
    vk::MemoryAllocateFlagsInfo alloc_flag_info = {};
    alloc_flag_info.pNext = dedicated_allocate_info;
    alloc_flag_info.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

    vk::MemoryPropertyFlags properties = {};
    if (memory_type == plRHIMemoryType::kDefault)
        properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    else if (memory_type == plRHIMemoryType::kUpload)
        properties = vk::MemoryPropertyFlagBits::eHostVisible;
    else if (memory_type == plRHIMemoryType::kReadback)
        properties = vk::MemoryPropertyFlagBits::eHostVisible;

    vk::MemoryAllocateInfo alloc_info = {};
    alloc_info.pNext = &alloc_flag_info;
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = device.FindMemoryType(memory_type_bits, properties);
    m_memory = device.GetDevice().allocateMemoryUnique(alloc_info);
}

plRHIMemoryType plVKMemory::GetMemoryType() const
{
    return m_memory_type;
}

vk::DeviceMemory plVKMemory::GetMemory() const
{
    return m_memory.get();
}
