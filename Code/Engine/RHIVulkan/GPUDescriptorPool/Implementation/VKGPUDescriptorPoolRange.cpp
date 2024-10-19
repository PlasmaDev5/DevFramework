#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPoolRange.h>
#include <RHIVulkan/GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h>

plVKGPUDescriptorPoolRange::plVKGPUDescriptorPoolRange(plVKGPUBindlessDescriptorPoolTyped& pool,
                                                   vk::DescriptorSet descriptor_set,
                                                   plUInt32 offset,
                                                   plUInt32 size,
                                                   vk::DescriptorType type)
    : m_pool(pool)
    , m_descriptor_set(descriptor_set)
    , m_offset(offset)
    , m_size(size)
    , m_type(type)
    //, m_Callback(this, [m_offset = m_offset, m_size = m_size, m_pool = m_pool](auto) { m_pool.get().OnRangeDestroy(m_offset, m_size); })
{
}

plVKGPUDescriptorPoolRange::~plVKGPUDescriptorPoolRange()
{
  m_pool.get().OnRangeDestroy(m_offset, m_size); 
}

vk::DescriptorSet plVKGPUDescriptorPoolRange::GetDescriptoSet() const
{
    return m_descriptor_set;
}

plUInt32 plVKGPUDescriptorPoolRange::GetOffset() const
{
    return m_offset;
}
