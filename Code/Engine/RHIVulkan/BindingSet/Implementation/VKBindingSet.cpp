#include <RHIVulkan/RHIVulkanPCH.h>

#include <Foundation/Containers/Map.h>
#include <RHIVulkan/BindingSet/VKBindingSet.h>
#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>
#include <RHIVulkan/Device/VKDevice.h>
#include <RHIVulkan/View/VKView.h>

plVKBindingSet::plVKBindingSet(plVKDevice& device, const plSharedPtr<plVKBindingSetLayout>& layout)
  : m_Device(device)
  , m_Layout(layout)
{
  decltype(auto) bindless_type = m_Layout->GetBindlessType();
  decltype(auto) descriptor_set_layouts = m_Layout->GetDescriptorSetLayouts();
  decltype(auto) descriptor_count_by_set = m_Layout->GetDescriptorCountBySet();
  for (plUInt32 i = 0; i < (plUInt32)descriptor_set_layouts.size(); ++i)
  {
    if (bindless_type.Contains(i))
    {
      m_DescriptorSets.PushBack(m_Device.GetGPUBindlessDescriptorPool(bindless_type.Find(i).Value())->GetDescriptorSet());
    }
    else
    {
      m_Descriptors.PushBack(m_Device.GetGPUDescriptorPool().AllocateDescriptorSet(descriptor_set_layouts[i].get(), descriptor_count_by_set[i]));
      m_DescriptorSets.PushBack(m_Descriptors.PeekBack().set.get());
    }
  }
}

void plVKBindingSet::WriteBindings(const std::vector<plRHIBindingDesc>& bindings)
{
  std::vector<vk::WriteDescriptorSet> descriptors;
  for (const auto& binding : bindings)
  {
    decltype(auto) vk_view = binding.view.Downcast<plVKView>();
    vk::WriteDescriptorSet descriptor = vk_view->GetDescriptor();
    descriptor.descriptorType = GetDescriptorType(binding.bindKey.viewType);
    descriptor.dstSet = m_DescriptorSets[binding.bindKey.space];
    descriptor.dstBinding = binding.bindKey.slot;
    descriptor.dstArrayElement = 0;
    descriptor.descriptorCount = 1;
    if (descriptor.pImageInfo || descriptor.pBufferInfo || descriptor.pTexelBufferView || descriptor.pNext)
    {
      descriptors.emplace_back(descriptor);
    }
  }

  if (!descriptors.empty())
  {
    m_Device.GetDevice().updateDescriptorSets((plUInt32)descriptors.size(), descriptors.data(), 0, nullptr);
  }
}

const plDynamicArray<vk::DescriptorSet>& plVKBindingSet::GetDescriptorSets() const
{
  return m_DescriptorSets;
}
