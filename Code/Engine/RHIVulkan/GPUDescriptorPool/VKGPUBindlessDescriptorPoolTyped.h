#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPoolRange.h>
#include <Foundation/Containers/ArrayMap.h>
#include <algorithm>

constexpr plUInt32 max_bindless_heap_size = 10000;

class plVKDevice;

class plVKGPUBindlessDescriptorPoolTyped
{
public:
  plVKGPUBindlessDescriptorPoolTyped(plVKDevice& device, vk::DescriptorType type);
  plVKGPUDescriptorPoolRange Allocate(plUInt32 count);
  void OnRangeDestroy(plUInt32 offset, plUInt32 size);
  vk::DescriptorSet GetDescriptorSet() const;

private:
  void ResizeHeap(plUInt32 req_size);

  plVKDevice& m_device;
  vk::DescriptorType m_type;
  plUInt32 m_size = 0;
  plUInt32 m_offset = 0;
  struct Descriptor
  {
    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSetLayout set_layout;
    vk::UniqueDescriptorSet set;
  } m_descriptor;
  plArrayMap<plUInt32, plUInt32> m_empty_ranges;
};
