#pragma once
#include <RHIVulkan/RHIVulkanDLL.h>

#include <RHI/BindingSetLayout/BindingSetLayout.h>

class plVKDevice;

class plVKBindingSetLayout
  : public plRHIBindingSetLayout
{
public:
  plVKBindingSetLayout(plVKDevice& device, const std::vector<plRHIBindKey>& descs);

  const plMap<plUInt32, vk::DescriptorType>& GetBindlessType() const;
  const std::vector<vk::UniqueDescriptorSetLayout>& GetDescriptorSetLayouts() const;
  const std::vector<plMap<vk::DescriptorType, size_t>>& GetDescriptorCountBySet() const;
  vk::PipelineLayout GetPipelineLayout() const;

private:
  plMap<plUInt32, vk::DescriptorType> m_bindless_type;
  std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts;
  std::vector<plMap<vk::DescriptorType, size_t>> m_descriptor_count_by_set;
  vk::UniquePipelineLayout m_pipeline_layout;
};

vk::DescriptorType GetDescriptorType(plRHIViewType view_type);
