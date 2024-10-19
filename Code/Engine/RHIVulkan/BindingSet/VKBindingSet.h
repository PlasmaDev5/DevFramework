#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/BindingSet/BindingSet.h>
#include <RHIVulkan/GPUDescriptorPool/VKGPUDescriptorPool.h>

class plVKDevice;
class plVKBindingSetLayout;

class plVKBindingSet
    : public plRHIBindingSet
{
public:
    plVKBindingSet(plVKDevice& device, const plSharedPtr<plVKBindingSetLayout>& layout);

    void WriteBindings(const std::vector<plRHIBindingDesc>& bindings) override;
    const plDynamicArray<vk::DescriptorSet>& GetDescriptorSets() const;

private:
    plVKDevice& m_Device;
    plDynamicArray<plVKDescriptorSetPool> m_Descriptors;
    plDynamicArray<vk::DescriptorSet> m_DescriptorSets;
    plSharedPtr<plVKBindingSetLayout> m_Layout;
};
