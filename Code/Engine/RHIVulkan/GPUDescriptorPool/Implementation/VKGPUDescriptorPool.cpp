#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPool.h>
#include <RHIVulkan/Device/VKDevice.h>

plVKGPUDescriptorPool::plVKGPUDescriptorPool(plVKDevice& device)
    : m_device(device)
{
}

vk::UniqueDescriptorPool plVKGPUDescriptorPool::CreateDescriptorPool(const plMap<vk::DescriptorType, size_t>& count)
{
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    for (auto & x : count)
    {
        pool_sizes.emplace_back();
        vk::DescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = x.Key();
        pool_size.descriptorCount = (plUInt32)x.Value();
    }

    // TODO: fix me
    if (count.IsEmpty())
    {
        pool_sizes.emplace_back();
        vk::DescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = vk::DescriptorType::eSampler;
        pool_size.descriptorCount = 1;
    }

    vk::DescriptorPoolCreateInfo pool_info = {};
    pool_info.poolSizeCount = (plUInt32)pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    return m_device.GetDevice().createDescriptorPoolUnique(pool_info);
}

plVKDescriptorSetPool plVKGPUDescriptorPool::AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout, const plMap<vk::DescriptorType, size_t>& count)
{
    plVKDescriptorSetPool res = {};
    res.pool = CreateDescriptorPool(count);

    vk::DescriptorSetAllocateInfo alloc_info = {};
    alloc_info.descriptorPool = res.pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &set_layout;
    auto descriptor_sets = m_device.GetDevice().allocateDescriptorSetsUnique(alloc_info);
    res.set = std::move(descriptor_sets.front());

    return res;
}
