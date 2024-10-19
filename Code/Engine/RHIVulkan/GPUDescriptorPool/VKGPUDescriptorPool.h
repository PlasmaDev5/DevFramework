#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>
#include <algorithm>

class plVKDevice;

struct plVKDescriptorSetPool
{
    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSet set;
};

class plVKGPUDescriptorPool
{
public:
    plVKGPUDescriptorPool(plVKDevice& device);
    plVKDescriptorSetPool AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout, const plMap<vk::DescriptorType, size_t>& count);

private:
    vk::UniqueDescriptorPool CreateDescriptorPool(const plMap<vk::DescriptorType, size_t>& count);

    plVKDevice& m_device;
};
