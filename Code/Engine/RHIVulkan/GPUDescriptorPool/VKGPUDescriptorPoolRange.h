#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <memory>

class plVKGPUBindlessDescriptorPoolTyped;

class plVKGPUDescriptorPoolRange
{
public:
  plVKGPUDescriptorPoolRange(plVKGPUBindlessDescriptorPoolTyped& pool,
    vk::DescriptorSet descriptor_set,
    plUInt32 offset,
    plUInt32 size,
    vk::DescriptorType type);
  ~plVKGPUDescriptorPoolRange();

  vk::DescriptorSet GetDescriptoSet() const;
  plUInt32 GetOffset() const;

private:
  std::reference_wrapper<plVKGPUBindlessDescriptorPoolTyped> m_pool;
  vk::DescriptorSet m_descriptor_set;
  plUInt32 m_offset;
  plUInt32 m_size;
  vk::DescriptorType m_type;
  //std::unique_ptr<VKGPUDescriptorPoolRange, std::function<void(VKGPUDescriptorPoolRange*)>> m_Callback;
};
