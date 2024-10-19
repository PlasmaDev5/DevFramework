#include <RHIVulkan/RHIVulkanPCH.h>

#include <RHIVulkan/BindingSetLayout/VKBindingSetLayout.h>
#include <RHIVulkan/Device/VKDevice.h>

vk::DescriptorType GetDescriptorType(plRHIViewType view_type)
{
    switch (view_type)
    {
      case plRHIViewType::kConstantBuffer:
        return vk::DescriptorType::eUniformBuffer;
      case plRHIViewType::kSampler:
        return vk::DescriptorType::eSampler;
      case plRHIViewType::kTexture:
        return vk::DescriptorType::eSampledImage;
      case plRHIViewType::kRWTexture:
        return vk::DescriptorType::eStorageImage;
      case plRHIViewType::kBuffer:
        return vk::DescriptorType::eUniformTexelBuffer;
      case plRHIViewType::kRWBuffer:
        return vk::DescriptorType::eStorageTexelBuffer;
      case plRHIViewType::kStructuredBuffer:
        return vk::DescriptorType::eStorageBuffer;
      case plRHIViewType::kRWStructuredBuffer:
        return vk::DescriptorType::eStorageBuffer;
      case plRHIViewType::kAccelerationStructure:
        return vk::DescriptorType::eAccelerationStructureKHR;
      default:
        break;
    }
    assert(false);
    return {};
}

vk::ShaderStageFlagBits ShaderType2Bit(plRHIShaderType type)
{
    switch (type)
    {
      case plRHIShaderType::kVertex:
        return vk::ShaderStageFlagBits::eVertex;
      case plRHIShaderType::kPixel:
        return vk::ShaderStageFlagBits::eFragment;
      case plRHIShaderType::kGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
      case plRHIShaderType::kCompute:
        return vk::ShaderStageFlagBits::eCompute;
      case plRHIShaderType::kAmplification:
        return vk::ShaderStageFlagBits::eTaskNV;
      case plRHIShaderType::kMesh:
        return vk::ShaderStageFlagBits::eMeshNV;
      case plRHIShaderType::kLibrary:
        return vk::ShaderStageFlagBits::eAll;
    }
    assert(false);
    return {};
}

plVKBindingSetLayout::plVKBindingSetLayout(plVKDevice& device, const std::vector<plRHIBindKey>& descs)
{
    plMap<plUInt32, std::vector<vk::DescriptorSetLayoutBinding>> bindings_by_set;
    plMap<plUInt32, std::vector<vk::DescriptorBindingFlags>> bindings_flags_by_set;

    for (const auto& bind_key : descs)
    {
        decltype(auto) binding = bindings_by_set[bind_key.space].emplace_back();
        binding.binding = bind_key.slot;
        binding.descriptorType = GetDescriptorType(bind_key.viewType);
        binding.descriptorCount = bind_key.count;
        binding.stageFlags = ShaderType2Bit(bind_key.shaderType);

        decltype(auto) binding_flag = bindings_flags_by_set[bind_key.space].emplace_back();
        if (bind_key.count == plMath::MaxValue<plUInt32>())
        {
            binding.descriptorCount = max_bindless_heap_size;
            binding_flag = vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
            m_bindless_type.Insert(bind_key.space, binding.descriptorType);
            binding.stageFlags = vk::ShaderStageFlagBits::eAll;
        }
    }

    for (const auto& set_desc : bindings_by_set)
    {
        vk::DescriptorSetLayoutCreateInfo layout_info = {};
      layout_info.bindingCount = (plUInt32)set_desc.Value().size();
        layout_info.pBindings = set_desc.Value().data();

        vk::DescriptorSetLayoutBindingFlagsCreateInfo layout_flags_info = {};
        layout_flags_info.bindingCount = (plUInt32)bindings_flags_by_set[set_desc.Key()].size();
        layout_flags_info.pBindingFlags = bindings_flags_by_set[set_desc.Key()].data();
        layout_info.pNext = &layout_flags_info;

        size_t set_num = set_desc.Key();
        if (m_descriptor_set_layouts.size() <= set_num)
        {
            m_descriptor_set_layouts.resize(set_num + 1);
            m_descriptor_count_by_set.resize(set_num + 1);
        }

        decltype(auto) descriptor_set_layout = m_descriptor_set_layouts[set_num];
        descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);

        decltype(auto) descriptor_count = m_descriptor_count_by_set[set_num];
        for (const auto& binding : set_desc.Value())
        {
            descriptor_count[binding.descriptorType] += binding.descriptorCount;
        }
    }

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
    for (auto& descriptor_set_layout : m_descriptor_set_layouts)
    {
        if (!descriptor_set_layout)
        {
            vk::DescriptorSetLayoutCreateInfo layout_info = {};
            descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);
        }

        descriptor_set_layouts.emplace_back(descriptor_set_layout.get());
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.setLayoutCount = (plUInt32)descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

    m_pipeline_layout = device.GetDevice().createPipelineLayoutUnique(pipeline_layout_info);
}

const plMap<plUInt32, vk::DescriptorType>& plVKBindingSetLayout::GetBindlessType() const
{
    return m_bindless_type;
}

const std::vector<vk::UniqueDescriptorSetLayout>& plVKBindingSetLayout::GetDescriptorSetLayouts() const
{
    return m_descriptor_set_layouts;
}

const std::vector<plMap<vk::DescriptorType, size_t>>& plVKBindingSetLayout::GetDescriptorCountBySet() const
{
    return m_descriptor_count_by_set;
}

vk::PipelineLayout plVKBindingSetLayout::GetPipelineLayout() const
{
    return m_pipeline_layout.get();
}
